# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.25

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /mnt/c/Users/Pete/source/repos/Proiect-PCD

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /mnt/c/Users/Pete/source/repos/Proiect-PCD

# Include any dependencies generated for this target.
include CMakeFiles/server.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/server.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/server.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/server.dir/flags.make

CMakeFiles/server.dir/src/server.c.o: CMakeFiles/server.dir/flags.make
CMakeFiles/server.dir/src/server.c.o: src/server.c
CMakeFiles/server.dir/src/server.c.o: CMakeFiles/server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/Pete/source/repos/Proiect-PCD/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/server.dir/src/server.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/server.dir/src/server.c.o -MF CMakeFiles/server.dir/src/server.c.o.d -o CMakeFiles/server.dir/src/server.c.o -c /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/server.c

CMakeFiles/server.dir/src/server.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/server.dir/src/server.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/server.c > CMakeFiles/server.dir/src/server.c.i

CMakeFiles/server.dir/src/server.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/server.dir/src/server.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/server.c -o CMakeFiles/server.dir/src/server.c.s

CMakeFiles/server.dir/src/interface_wrapper.cpp.o: CMakeFiles/server.dir/flags.make
CMakeFiles/server.dir/src/interface_wrapper.cpp.o: src/interface_wrapper.cpp
CMakeFiles/server.dir/src/interface_wrapper.cpp.o: CMakeFiles/server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/Pete/source/repos/Proiect-PCD/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/server.dir/src/interface_wrapper.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/server.dir/src/interface_wrapper.cpp.o -MF CMakeFiles/server.dir/src/interface_wrapper.cpp.o.d -o CMakeFiles/server.dir/src/interface_wrapper.cpp.o -c /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/interface_wrapper.cpp

CMakeFiles/server.dir/src/interface_wrapper.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/server.dir/src/interface_wrapper.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/interface_wrapper.cpp > CMakeFiles/server.dir/src/interface_wrapper.cpp.i

CMakeFiles/server.dir/src/interface_wrapper.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/server.dir/src/interface_wrapper.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/interface_wrapper.cpp -o CMakeFiles/server.dir/src/interface_wrapper.cpp.s

CMakeFiles/server.dir/src/login.c.o: CMakeFiles/server.dir/flags.make
CMakeFiles/server.dir/src/login.c.o: src/login.c
CMakeFiles/server.dir/src/login.c.o: CMakeFiles/server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/Pete/source/repos/Proiect-PCD/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/server.dir/src/login.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/server.dir/src/login.c.o -MF CMakeFiles/server.dir/src/login.c.o.d -o CMakeFiles/server.dir/src/login.c.o -c /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/login.c

CMakeFiles/server.dir/src/login.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/server.dir/src/login.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/login.c > CMakeFiles/server.dir/src/login.c.i

CMakeFiles/server.dir/src/login.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/server.dir/src/login.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/login.c -o CMakeFiles/server.dir/src/login.c.s

CMakeFiles/server.dir/src/image_prc_wrapper.cpp.o: CMakeFiles/server.dir/flags.make
CMakeFiles/server.dir/src/image_prc_wrapper.cpp.o: src/image_prc_wrapper.cpp
CMakeFiles/server.dir/src/image_prc_wrapper.cpp.o: CMakeFiles/server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/Pete/source/repos/Proiect-PCD/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/server.dir/src/image_prc_wrapper.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/server.dir/src/image_prc_wrapper.cpp.o -MF CMakeFiles/server.dir/src/image_prc_wrapper.cpp.o.d -o CMakeFiles/server.dir/src/image_prc_wrapper.cpp.o -c /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/image_prc_wrapper.cpp

CMakeFiles/server.dir/src/image_prc_wrapper.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/server.dir/src/image_prc_wrapper.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/image_prc_wrapper.cpp > CMakeFiles/server.dir/src/image_prc_wrapper.cpp.i

CMakeFiles/server.dir/src/image_prc_wrapper.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/server.dir/src/image_prc_wrapper.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/image_prc_wrapper.cpp -o CMakeFiles/server.dir/src/image_prc_wrapper.cpp.s

CMakeFiles/server.dir/src/fistic_wrapper.cpp.o: CMakeFiles/server.dir/flags.make
CMakeFiles/server.dir/src/fistic_wrapper.cpp.o: src/fistic_wrapper.cpp
CMakeFiles/server.dir/src/fistic_wrapper.cpp.o: CMakeFiles/server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/Pete/source/repos/Proiect-PCD/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/server.dir/src/fistic_wrapper.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/server.dir/src/fistic_wrapper.cpp.o -MF CMakeFiles/server.dir/src/fistic_wrapper.cpp.o.d -o CMakeFiles/server.dir/src/fistic_wrapper.cpp.o -c /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/fistic_wrapper.cpp

CMakeFiles/server.dir/src/fistic_wrapper.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/server.dir/src/fistic_wrapper.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/fistic_wrapper.cpp > CMakeFiles/server.dir/src/fistic_wrapper.cpp.i

CMakeFiles/server.dir/src/fistic_wrapper.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/server.dir/src/fistic_wrapper.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/fistic_wrapper.cpp -o CMakeFiles/server.dir/src/fistic_wrapper.cpp.s

# Object files for target server
server_OBJECTS = \
"CMakeFiles/server.dir/src/server.c.o" \
"CMakeFiles/server.dir/src/interface_wrapper.cpp.o" \
"CMakeFiles/server.dir/src/login.c.o" \
"CMakeFiles/server.dir/src/image_prc_wrapper.cpp.o" \
"CMakeFiles/server.dir/src/fistic_wrapper.cpp.o"

# External object files for target server
server_EXTERNAL_OBJECTS =

server: CMakeFiles/server.dir/src/server.c.o
server: CMakeFiles/server.dir/src/interface_wrapper.cpp.o
server: CMakeFiles/server.dir/src/login.c.o
server: CMakeFiles/server.dir/src/image_prc_wrapper.cpp.o
server: CMakeFiles/server.dir/src/fistic_wrapper.cpp.o
server: CMakeFiles/server.dir/build.make
server: /usr/local/lib/libopencv_gapi.so.4.9.0
server: /usr/local/lib/libopencv_highgui.so.4.9.0
server: /usr/local/lib/libopencv_ml.so.4.9.0
server: /usr/local/lib/libopencv_objdetect.so.4.9.0
server: /usr/local/lib/libopencv_photo.so.4.9.0
server: /usr/local/lib/libopencv_stitching.so.4.9.0
server: /usr/local/lib/libopencv_video.so.4.9.0
server: /usr/local/lib/libopencv_videoio.so.4.9.0
server: /usr/lib/x86_64-linux-gnu/libpq.so
server: libdatabase.so
server: liblogin.so
server: /usr/local/lib/libopencv_imgcodecs.so.4.9.0
server: /usr/local/lib/libopencv_dnn.so.4.9.0
server: /usr/local/lib/libopencv_calib3d.so.4.9.0
server: /usr/local/lib/libopencv_features2d.so.4.9.0
server: /usr/local/lib/libopencv_flann.so.4.9.0
server: /usr/local/lib/libopencv_imgproc.so.4.9.0
server: /usr/local/lib/libopencv_core.so.4.9.0
server: CMakeFiles/server.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/mnt/c/Users/Pete/source/repos/Proiect-PCD/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Linking CXX executable server"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/server.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/server.dir/build: server
.PHONY : CMakeFiles/server.dir/build

CMakeFiles/server.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/server.dir/cmake_clean.cmake
.PHONY : CMakeFiles/server.dir/clean

CMakeFiles/server.dir/depend:
	cd /mnt/c/Users/Pete/source/repos/Proiect-PCD && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/c/Users/Pete/source/repos/Proiect-PCD /mnt/c/Users/Pete/source/repos/Proiect-PCD /mnt/c/Users/Pete/source/repos/Proiect-PCD /mnt/c/Users/Pete/source/repos/Proiect-PCD /mnt/c/Users/Pete/source/repos/Proiect-PCD/CMakeFiles/server.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/server.dir/depend

