# -*- mode: cmake; -*-
find_program( CLANG_CC clang )
find_program( CLANG_CXX clang++ )
set(ENV{CC} ${CLANG_CC})
set(ENV{CXX} ${CLANG_CXX})
set(COMPILER_ID "clang")

set(CTEST_PUSH_PACKAGES 0)
get_filename_component(_currentDir "${CMAKE_CURRENT_LIST_FILE}" PATH)
include( "${_currentDir}/VZLoggerNightly.cmake")
