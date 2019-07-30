# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /db/storage/Development/Projects/Trinity/Switch/ext/FastPFor

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /db/storage/Development/Projects/Trinity/Switch/ext/FastPFor

# Include any dependencies generated for this target.
include CMakeFiles/codecssnappy.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/codecssnappy.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/codecssnappy.dir/flags.make

CMakeFiles/codecssnappy.dir/src/codecs.cpp.o: CMakeFiles/codecssnappy.dir/flags.make
CMakeFiles/codecssnappy.dir/src/codecs.cpp.o: src/codecs.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/db/storage/Development/Projects/Trinity/Switch/ext/FastPFor/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/codecssnappy.dir/src/codecs.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/codecssnappy.dir/src/codecs.cpp.o -c /db/storage/Development/Projects/Trinity/Switch/ext/FastPFor/src/codecs.cpp

CMakeFiles/codecssnappy.dir/src/codecs.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/codecssnappy.dir/src/codecs.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /db/storage/Development/Projects/Trinity/Switch/ext/FastPFor/src/codecs.cpp > CMakeFiles/codecssnappy.dir/src/codecs.cpp.i

CMakeFiles/codecssnappy.dir/src/codecs.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/codecssnappy.dir/src/codecs.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /db/storage/Development/Projects/Trinity/Switch/ext/FastPFor/src/codecs.cpp -o CMakeFiles/codecssnappy.dir/src/codecs.cpp.s

CMakeFiles/codecssnappy.dir/src/codecs.cpp.o.requires:

.PHONY : CMakeFiles/codecssnappy.dir/src/codecs.cpp.o.requires

CMakeFiles/codecssnappy.dir/src/codecs.cpp.o.provides: CMakeFiles/codecssnappy.dir/src/codecs.cpp.o.requires
	$(MAKE) -f CMakeFiles/codecssnappy.dir/build.make CMakeFiles/codecssnappy.dir/src/codecs.cpp.o.provides.build
.PHONY : CMakeFiles/codecssnappy.dir/src/codecs.cpp.o.provides

CMakeFiles/codecssnappy.dir/src/codecs.cpp.o.provides.build: CMakeFiles/codecssnappy.dir/src/codecs.cpp.o


# Object files for target codecssnappy
codecssnappy_OBJECTS = \
"CMakeFiles/codecssnappy.dir/src/codecs.cpp.o"

# External object files for target codecssnappy
codecssnappy_EXTERNAL_OBJECTS =

codecssnappy: CMakeFiles/codecssnappy.dir/src/codecs.cpp.o
codecssnappy: CMakeFiles/codecssnappy.dir/build.make
codecssnappy: libFastPFor.a
codecssnappy: /usr/lib/x86_64-linux-gnu/libsnappy.so
codecssnappy: CMakeFiles/codecssnappy.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/db/storage/Development/Projects/Trinity/Switch/ext/FastPFor/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable codecssnappy"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/codecssnappy.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/codecssnappy.dir/build: codecssnappy

.PHONY : CMakeFiles/codecssnappy.dir/build

CMakeFiles/codecssnappy.dir/requires: CMakeFiles/codecssnappy.dir/src/codecs.cpp.o.requires

.PHONY : CMakeFiles/codecssnappy.dir/requires

CMakeFiles/codecssnappy.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/codecssnappy.dir/cmake_clean.cmake
.PHONY : CMakeFiles/codecssnappy.dir/clean

CMakeFiles/codecssnappy.dir/depend:
	cd /db/storage/Development/Projects/Trinity/Switch/ext/FastPFor && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /db/storage/Development/Projects/Trinity/Switch/ext/FastPFor /db/storage/Development/Projects/Trinity/Switch/ext/FastPFor /db/storage/Development/Projects/Trinity/Switch/ext/FastPFor /db/storage/Development/Projects/Trinity/Switch/ext/FastPFor /db/storage/Development/Projects/Trinity/Switch/ext/FastPFor/CMakeFiles/codecssnappy.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/codecssnappy.dir/depend

