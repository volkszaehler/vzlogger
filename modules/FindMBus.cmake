# -*- mode: cmake; -*-
# - Try to find libmbus include dirs and libraries
# Usage of this module as follows:
# This file defines:
# * MBUS_FOUND if protoc was found
# * MBUS_LIBRARY The lib to link to (currently only a static unix lib, not
# portable)
# * MBUS_INCLUDE_DIR The include directories for libmbus.

message(STATUS "FindMBus check")
IF (NOT WIN32)
  include(FindPkgConfig)
  if ( PKG_CONFIG_FOUND )

     pkg_check_modules (PC_MBUS libmbus>=0.8.0)

     set(MBUS_DEFINITIONS ${PC_MBUS_CFLAGS_OTHER})
  endif(PKG_CONFIG_FOUND)
endif (NOT WIN32)

message(STATUS "Looking for libmbus in ${PC_MBUS_INCLUDEDIR}")

# find the include files
FIND_PATH(MBUS_INCLUDE_DIR mbus/mbus.h
   HINTS
     ${PC_MBUS_INCLUDEDIR}
     ${PC_MBUS_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
)

# locate the library
IF(WIN32)
  SET(MBUS_LIBRARY_NAMES ${MBUS_LIBRARY_NAMES} libmbus.lib)
ELSE(WIN32)
  SET(MBUS_LIBRARY_NAMES ${MBUS_LIBRARY_NAMES} mbus)
ENDIF(WIN32)
FIND_LIBRARY(MBUS_LIBRARY NAMES ${MBUS_LIBRARY_NAMES}
  HINTS
    ${PC_MBUS_LIBDIR}
    ${PC_MBUS_LIBRARY_DIRS}
)

# if the include and the program are found then we have it
IF(MBUS_INCLUDE_DIR AND MBUS_LIBRARY)
  SET(MBUS_FOUND "YES")
  message("libmbus found: '${MBUS_INCLUDE_DIR}'")
#  SET(MBUS_INCLUDE_DIR ${MBUS-C_INCLUDE_DIR})
ENDIF(MBUS_INCLUDE_DIR AND MBUS_LIBRARY)

if( NOT WIN32)
  list(APPEND MBUS_LIBRARY "-lm")
endif( NOT WIN32)

MARK_AS_ADVANCED(
  MBUS_FOUND
  MBUS_LIBRARY
  MBUS_INCLUDE_DIR
)
