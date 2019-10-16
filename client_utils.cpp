#include "tank_client.h"

void TankClient::wait_scheduled(const uint32_t req_id) {
        TANK_EXPECT(req_id);

        while (should_poll()) {
                poll(1000);

                if (unlikely(!faults().empty())) {
                        throw Switch::data_error("Fault while waiting responses");
                }

                for (const auto &it : produce_acks()) {
                        if (it.clientReqId == req_id) {
                                return;
                        }
                }

                for (const auto &it : consumed()) {
                        if (it.clientReqId == req_id) {
                                return;
                        }
                }
        }
}

uint64_t TankClient::sequence_number_by_event_time(const topic_partition &topic_partition, const uint64_t event_time, const uint64_t cut_off_threshold) {
	// @robert's idea
        if (should_poll()) {
                // this utility function is going to affect all requests scheduled
                // so one should't invoke this function unless there are no pending responses
                throw std::runtime_error("Requests already in progress");
        }

        // First, discover partitions in order to determine the [first, last] sequence numbers reange of the partition
        tl::optional<std::pair<uint64_t, uint64_t>> offsets_space;
        static constexpr const bool                 trace{false};

        if (discover_partitions(topic_partition.first) == 0) {
                throw std::runtime_error("Failed to request topic's partitions list");
        }

        if (trace) {
                SLog("Determining partitions list for ", topic_partition.first, "/", topic_partition.second, "\n");
        }

        while (should_poll()) {
                try {
                        poll(1000);
                } catch (...) {
                        throw;
                }

                if (!faults().empty()) {
                        throw std::runtime_error("Unable to discover partitions - request failed");
                }

                if (!discovered_partitions().empty()) {
                        const auto &it = discovered_partitions().front();

                        EXPECT(discovered_partitions().size() == 1);
                        EXPECT(it.topic == topic_partition.first);

                        if (topic_partition.second >= it.watermarks.size()) {
                                throw std::runtime_error("Partition not defined in topic partitions");
                        }

                        offsets_space = it.watermarks.offset[topic_partition.second];
                        break;
                }
        }

        if (!offsets_space) {
                throw std::runtime_error("Unable to determine offsets space for (topic, partition)");
        }

        // use binary search to determine the appropriate base sequence number
        const auto v = offsets_space.value();

        if (v.second == v.first) {
                return v.first;
        }

        uint64_t       low          = v.first;
        uint64_t       high         = v.second - 1;
        const uint64_t cut_off      = event_time - cut_off_threshold;

        while (low <= high) {
                // guard against overflow
                // i.e don't use (low + high) / 2
                uint64_t msg_ts;
                auto     mid = low + (high - low) / 2;

                if (trace) {
                        SLog("low = ", low, ", high = ", high, ", mid = ", mid, "\n");
                }

                // determine the timestamp of the first message with (seq_num >= mid)
                for (uint32_t req_id{0}, fetch_size{32 * 1024};;) {
                        if (!req_id) {
                                req_id = consume_from(topic_partition, mid, fetch_size, 0, 0);
                                if (!req_id) {
                                        reset();
                                        continue;
                                }
                        }

                        try {
                                poll(2000);
                        } catch (...) {
                                throw;
                        }

                        if (!faults().empty()) {
                                for (const auto &it : faults()) {
                                        if (it.type == fault::Type::BoundaryCheck) {
                                                mid = it.adjust_seqnum_by_boundaries(mid);
                                        } else {
                                                throw std::runtime_error("Failed to access message");
                                        }
                                }

                                req_id = 0;
                                continue;
                        }

                        if (consumed().empty()) {
                                continue;
                        }

                        const auto &part = consumed().front();

                        if (!part.msgs.empty()) {
                                msg_ts = part.msgs.offset->ts;
                                if (trace) {
                                        SLog("Got ", Date::ts_repr(Timings::Milliseconds::ToSeconds(msg_ts)), " need ", Date::ts_repr(Timings::Milliseconds::ToSeconds(event_time)), "\n");
                                }
                                break;
                        }

                        req_id     = 0;
                        fetch_size = std::max<uint32_t>(fetch_size, part.next.min_fetch_size);
                        mid        = part.next.seq_num;
                }

                if (msg_ts < event_time && msg_ts >= cut_off) {
                        if (trace) {
                                SLog("Stopping now at ", Date::ts_repr(msg_ts / 1000), ", event_time = ", Date::ts_repr(event_time / 1000), " ", Date::ts_repr(cut_off / 1000), "\n");
                        }
                        break;
                }

                if (event_time < msg_ts) {
                        high = mid - 1;
                } else {
                        low = mid + 1;
                }
        }

        return low;
}