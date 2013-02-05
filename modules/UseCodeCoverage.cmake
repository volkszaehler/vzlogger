# -*- mode: cmake; -*-
# - Enable Code Coverage
#
#
# USAGE:
# 1. Copy this file into your cmake modules path
# 2. Add the following line to your CMakeLists.txt:
#      include(UseCodeCoverage)
# 3. Select the ENABLE_CODECOVERAGE option when you want to build with code coverage enabled.
#
# Variables you may define are:
#  CODECOV_OUTPUTFILE - the name of the temporary output file used. Defaults to "cmake_coverage.output"
#  CODECOV_HTMLOUTPUTDIR - the name of the directory where HTML results are placed. Defaults to "coverage_results"
#

#
#  Copyright (C) 2010 Brad Hards <bradh@frogmouth.net>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

OPTION( ENABLE_CODECOVERAGE "Enable code coverage testing support" )

if ( ENABLE_CODECOVERAGE )

  if ( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "Profile" )
    message( WARNING "Code coverage results with an optimised (non-Debug) build may be misleading" )
  endif ( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "Profile" )

  if ( NOT DEFINED CODECOV_OUTPUTFILE )
    set( CODECOV_OUTPUTFILE cmake_coverage.output )
    set( CODECOVB_OUTPUTFILE cmake_coverage.base.output )
    set( CODECOVT_OUTPUTFILE cmake_coverage.test.output )
  endif ( NOT DEFINED CODECOV_OUTPUTFILE )

  if ( NOT DEFINED CODECOV_HTMLOUTPUTDIR )
    set( CODECOV_HTMLOUTPUTDIR coverage_results )
  endif ( NOT DEFINED CODECOV_HTMLOUTPUTDIR )

  if ( CMAKE_COMPILER_IS_GNUCXX )
    find_program( CODECOV_GCOV gcov )
    find_program( CODECOV_LCOV lcov )
    find_program( CODECOV_GENHTML genhtml )
    add_definitions( -fprofile-arcs -ftest-coverage )
    add_definitions( -ftime-report -fmem-report )
    link_libraries( gcov )
    #set( CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} --coverage )
    add_custom_target( coverage_init ${CODECOV_LCOV} --base-directory ${CMAKE_SOURCE_DIR} --directory ${CMAKE_BINARY_DIR} --output-file ${CODECOVB_OUTPUTFILE} --capture --initial )
    add_custom_target( coverage ${CODECOV_LCOV} --base-directory ${CMAKE_SOURCE_DIR} --directory ${CMAKE_BINARY_DIR} --output-file ${CODECOVT_OUTPUTFILE} --capture
      COMMAND ${CODECOV_LCOV} -a ${CODECOVB_OUTPUTFILE} -a ${CODECOVT_OUTPUTFILE} -o ${CODECOV_OUTPUTFILE}
      COMMAND genhtml -o ${CODECOV_HTMLOUTPUTDIR} ${CODECOV_OUTPUTFILE} )
    add_dependencies(coverage coverage_init)
    #       add_dependencies(test coverage_init)
  endif ( CMAKE_COMPILER_IS_GNUCXX )

endif (ENABLE_CODECOVERAGE )
