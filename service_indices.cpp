#include "service_common.h"

// return the absolute sequence number of the last message in the segment
uint64_t Service::segment_lastmsg_seqnum(int log_fd, const uint64_t base_seqnum) {
        const auto file_size = lseek(log_fd, 0, SEEK_END);
        const auto vma_dtor  = [file_size](void *ptr) noexcept {
                if (ptr && ptr != MAP_FAILED) {
                        munmap(ptr, file_size);
                }
        };
        std::unique_ptr<void, decltype(vma_dtor)> vma(mmap(nullptr, file_size, PROT_READ, MAP_SHARED, log_fd, 0), vma_dtor);
        auto                                      file_data = vma.get();
        auto                                      next      = base_seqnum;

        if (file_data == MAP_FAILED) {
                throw Switch::system_error("mmap() failed");
        }

        madvise(file_data, file_size, MADV_SEQUENTIAL | MADV_DONTDUMP);

        for (const auto *p = static_cast<const uint8_t *>(file_data), *const e = p + file_size; p < e;) {
                const auto     bundle_size      = Compression::decode_varuint32(p);
                const auto     next_bundle      = p + bundle_size;
                const auto     bundle_hdr_flags = decode_pod<uint8_t>(p);
                const bool     sparse_bundle    = bundle_hdr_flags & (1u << 6);
                const uint32_t msgset_size      = ((bundle_hdr_flags >> 2) & 0xf) ?: Compression::decode_varuint32(p);

                if (sparse_bundle) {
                        const auto first_msg_seqnum = decode_pod<uint64_t>(p);
                        uint64_t   last_msg_seqnum;

                        if (msgset_size != 1) {
                                last_msg_seqnum = first_msg_seqnum + Compression::decode_varuint32(p) + 1;
                        } else {
                                last_msg_seqnum = first_msg_seqnum;
                        }

                        next = last_msg_seqnum + 1;
                } else {
                        next += msgset_size;
                }

                p = next_bundle;
        }

        return next > base_seqnum ? next - 1 : base_seqnum;
}

// we need to be able to repair a segment in case e.g the msg size (i.e messages in the bundle) is corrupt
// and in that case, we need to be able to use to set bundle size to 1.e 1, and then switch to a sparse bundle for the next
// bundle, so that we can effectively skip the bogus bundle
void Service::rebuild_index(int logFd, int indexFd, const uint64_t base_seqnum) {
        enum {
                trace = false,
        };
        const auto fileSize = lseek64(logFd, 0, SEEK_END);

        if (fileSize == -1) {
                throw Switch::system_error("Unable to access file:", strerror(errno));
        }

        auto *const                   fileData = mmap(nullptr, fileSize, PROT_READ, MAP_SHARED, logFd, 0);
        IOBuffer                      b;
        uint32_t                      next = 0;
        static constexpr const size_t step{4096};        // TODO: respect configuration
        uint64_t                      firstMsgSeqNum{0}; // this is the _relative_ sequence number
        std::size_t                   bundles{0}, sparse_bundles{0};
        std::size_t                   _max       = 0u;
        std::size_t                   _max_bytes = 0u;

        static const auto checked_msgs_size = [](const uint64_t msg_size, const std::size_t bundle_len) -> uint32_t {
                if (msg_size > bundle_len) {
                        Print(ansifmt::bold, ansifmt::color_red, "Unexpected msg_size ", msg_size, " for bundle_len ", bundle_len, ansifmt::reset, "\n");

                        return 1u;
                }

                return msg_size;
        };

        if (fileData == MAP_FAILED) {
                throw Switch::system_error("Unable to mmap():", strerror(errno));
        }

        DEFER({ munmap(fileData, fileSize); });
        madvise(fileData, fileSize, MADV_SEQUENTIAL | MADV_DONTDUMP);

        Print(ansifmt::bold, ansifmt::color_brown, "Rebuilding index of log of size ", size_repr(fileSize), " with base seq.number ", base_seqnum, ansifmt::reset, " ..\n");

        for (const auto *      p         = static_cast<const uint8_t *>(fileData),
                        *const e         = p + fileSize,
                        *const base      = p,
                        *next_checkpoint = p;
             p < e;) {
                const auto bundle_base = p;
                const auto bundleLen   = Compression::decode_varuint32(p);
                const auto next_bundle = p + bundleLen;

                if (unlikely(next_bundle > e)) {
                        if (getenv("TANK_FORCE_SALVAGE_CURSEGMENT")) {
                                if (ftruncate(indexFd, 0) == -1) {
                                        Print("Failed to truncate the index:", strerror(errno), "\n");
                                        exit(1);
                                } else {
                                        Print("Please restart tank\n");
                                        exit(0);
                                }
                        } else {
                                Print("Likely corrupt segment(ran out of disk space?).\n");
                                Print("Set ", ansifmt::bold, "TANK_FORCE_SALVAGE_CURSEGMENT=1", ansifmt::reset, " and restart Tank so that\n");
                                Print("it will _delete_ the current segment index, and then you will need to restart again for Tank to get a chance to salvage the segment data\n");
                                Print("Will not index any further\n");
                                exit(1);
                        }
                }

                TANK_EXPECT(p < e);

                const auto     bundle_hdr_flags = decode_pod<uint8_t>(p);
                const bool     sparse_bundle    = bundle_hdr_flags & (1u << 6);
                const uint32_t msgset_size      = checked_msgs_size(((bundle_hdr_flags >> 2) & 0xf) ?: Compression::decode_varuint32(p), bundleLen);
                const auto     maybe_bogus      = msgset_size > bundleLen; // this can't possibly be
                const auto     deep_trace       = maybe_bogus;

                TANK_EXPECT(p <= e);

                _max       = std::max<std::size_t>(_max, msgset_size);
                _max_bytes = std::max<std::size_t>(_max_bytes, bundleLen);

                if (deep_trace) {
                        SLog("New bundle bundle_hdr_flags = ", bundle_hdr_flags,
                             ", msgSetSize = ", msgset_size,
                             ", bundleLen = ", bundleLen, " (", size_repr(bundleLen), ")",
                             ", sparse_bundle = ", sparse_bundle, "\n");
                }

                if (sparse_bundle) {
                        uint64_t   lastMsgSeqNum;
                        const auto _saved = next;

                        firstMsgSeqNum = decode_pod<uint64_t>(p) - base_seqnum; // XXX: important, index encodes _relative_ sequence numbers

                        if (msgset_size != 1) {
                                lastMsgSeqNum = firstMsgSeqNum + Compression::decode_varuint32(p) + 1;
                        } else {
                                lastMsgSeqNum = firstMsgSeqNum;
                        }

                        if (trace) {
                                SLog("bundle's sparse first (rel) ", firstMsgSeqNum, ", last (rel) ", lastMsgSeqNum, ", would have been ", _saved, "\n");
                        }

                        next = lastMsgSeqNum + 1;
                        ++sparse_bundles;
                } else {
                        firstMsgSeqNum = next;
                        next += msgset_size;
                }

                if (deep_trace) {
                        SLog("First message of deep traced bundle is ", firstMsgSeqNum, ", next is:", firstMsgSeqNum + msgset_size, "\n");
                }


                if (p >= next_checkpoint or maybe_bogus) {
                        const auto size = std::distance(base, bundle_base);

                        if (maybe_bogus) {
                                SLog("Indexing first msg seqnum = ", firstMsgSeqNum, "(", firstMsgSeqNum + base_seqnum, ")  size = ", size, " (", size_repr(size), ")\n");
                        }

                        b.pack(static_cast<uint32_t>(firstMsgSeqNum),
                               static_cast<uint32_t>(size));

			if (maybe_bogus) {
				next_checkpoint = next_bundle;
			} else {
				next_checkpoint = bundle_base + step;
			}
                }


                p = next_bundle;
                TANK_EXPECT(p <= e);
                ++bundles;
        }

        const auto last_seqnum = next ? next - 1 : 0;

        Print("Rebuilt index of size ", size_repr(b.size()),
              ", last msg relative seq.num ", last_seqnum,
              "(first_abseqnum = ", base_seqnum,
              ", last_abseqnum = ", base_seqnum + last_seqnum,
              "), largest bundle size ", _max,
              ", largest bundle in bytes ", size_repr(_max_bytes), "\n");
        Print(dotnotation_repr(bundles), " bundles processed ", dotnotation_repr(sparse_bundles), " sparse\n");

        if (pwrite64(indexFd, b.data(), b.size(), 0) != b.size()) {
                throw Switch::system_error("Failed to store index:", strerror(errno));
        }

        if (ftruncate(indexFd, b.size())) {
                throw Switch::system_error("Failed to truncate index file:", strerror(errno));
        }

        fsync(indexFd);

        if (trace) {
                SLog("REBUILT INDEX\n");
        }
}

void Service::verify_index(int fd, const bool wideEntries) {
        static constexpr bool trace{false};
        const auto            fileSize = lseek64(fd, 0, SEEK_END);

        if (!fileSize) {
                return;
        } else if (fileSize & 7) {
                throw Switch::system_error("Unexpected index filesize");
        }

        auto *const fileData = mmap(nullptr, fileSize, PROT_READ, MAP_SHARED, fd, 0);

        if (fileData == MAP_FAILED)
                throw Switch::system_error("Unable to mmap():", strerror(errno));

        DEFER(
            {
                    madvise(fileData, fileSize, MADV_DONTNEED);
                    munmap(fileData, fileSize);
            });

        madvise(fileData, fileSize, MADV_SEQUENTIAL | MADV_DONTDUMP);

        if (wideEntries) {
                IMPLEMENT_ME();
        }

        for (const auto *      p = reinterpret_cast<const uint32_t *>(fileData),
                        *const e = p + (fileSize / sizeof(uint32_t)), *const b = p;
             p != e; p += 2) {
                if (trace) {
                        SLog("Entry(", *p, ", ", p[1], ")\n");
                }

                if (p != b) {
                        if (unlikely(p[0] <= p[-2])) {
                                Print("Unexpected rel.seq.num ", p[0], ", should have been > ", p[-2], ", at entry ", dotnotation_repr((p - static_cast<uint32_t *>(fileData)) / 2), " of index of ", dotnotation_repr(fileSize / (sizeof(uint32_t) + sizeof(uint32_t))), " entries\n");
                                throw Switch::system_error("Corrupt Index");
                        }

                        if (unlikely(p[1] <= p[-1])) {
                                Print("Unexpected file offset ", p[0], ", should have been > ", p[-2], ", at entry ", dotnotation_repr((p - static_cast<uint32_t *>(fileData)) / 2), " of index of ", dotnotation_repr(fileSize / (sizeof(uint32_t) + sizeof(uint32_t))), " entries\n");
                                throw Switch::system_error("Corrupt Index");
                        }
                } else {
                        if (p[0] != 0 || p[1] != 0) {
                                Print("Expected first entry to be (0, 0)\n");
                                throw Switch::system_error("Corrupt Index");
                        }
                }
        }
}

