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
include CMakeFiles/login.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/login.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/login.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/login.dir/flags.make

CMakeFiles/login.dir/src/login.c.o: CMakeFiles/login.dir/flags.make
CMakeFiles/login.dir/src/login.c.o: src/login.c
CMakeFiles/login.dir/src/login.c.o: CMakeFiles/login.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/Pete/source/repos/Proiect-PCD/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/login.dir/src/login.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/login.dir/src/login.c.o -MF CMakeFiles/login.dir/src/login.c.o.d -o CMakeFiles/login.dir/src/login.c.o -c /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/login.c

CMakeFiles/login.dir/src/login.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/login.dir/src/login.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/login.c > CMakeFiles/login.dir/src/login.c.i

CMakeFiles/login.dir/src/login.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/login.dir/src/login.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/c/Users/Pete/source/repos/Proiect-PCD/src/login.c -o CMakeFiles/login.dir/src/login.c.s

# Object files for target login
login_OBJECTS = \
"CMakeFiles/login.dir/src/login.c.o"

# External object files for target login
login_EXTERNAL_OBJECTS =

liblogin.so: CMakeFiles/login.dir/src/login.c.o
liblogin.so: CMakeFiles/login.dir/build.make
liblogin.so: /usr/lib/x86_64-linux-gnu/libpq.so
liblogin.so: CMakeFiles/login.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/mnt/c/Users/Pete/source/repos/Proiect-PCD/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C shared library liblogin.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/login.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/login.dir/build: liblogin.so
.PHONY : CMakeFiles/login.dir/build

CMakeFiles/login.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/login.dir/cmake_clean.cmake
.PHONY : CMakeFiles/login.dir/clean

CMakeFiles/login.dir/depend:
	cd /mnt/c/Users/Pete/source/repos/Proiect-PCD && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/c/Users/Pete/source/repos/Proiect-PCD /mnt/c/Users/Pete/source/repos/Proiect-PCD /mnt/c/Users/Pete/source/repos/Proiect-PCD /mnt/c/Users/Pete/source/repos/Proiect-PCD /mnt/c/Users/Pete/source/repos/Proiect-PCD/CMakeFiles/login.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/login.dir/depend
