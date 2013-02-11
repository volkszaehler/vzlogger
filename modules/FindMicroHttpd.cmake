# -*- mode: cmake; -*-
# - Try to find libmicrohttpd include dirs and libraries
# Usage of this module as follows:
# This file defines:
# * MICROHTTPD_FOUND if protoc was found
# * MICROHTTPD_LIBRARY The lib to link to (currently only a static unix lib, not
# portable)
# * MICROHTTPD_INCLUDE The include directories for libmicrohttpd.

message(STATUS "FindMicrohttpd check")
IF (NOT WIN32)
  include(FindPkgConfig)
  if ( PKG_CONFIG_FOUND )

     pkg_check_modules (PC_MICROHTTPD microhttpd>=0.9)

     set(MICROHTTPD_DEFINITIONS ${PC_MICROHTTPD_CFLAGS_OTHER})
  endif(PKG_CONFIG_FOUND)
endif (NOT WIN32)

#
# set defaults
SET(_microhttpd_HOME "/usr/local")
SET(_microhttpd_INCLUDE_SEARCH_DIRS
  ${CMAKE_INCLUDE_PATH}
  /usr/local/include
  /usr/include
  )

SET(_microhttpd_LIBRARIES_SEARCH_DIRS
  ${CMAKE_LIBRARY_PATH}
  /usr/local/lib
  /usr/lib
  )

##
if( "${MICROHTTPD_HOME}" STREQUAL "")
  if("" MATCHES "$ENV{MICROHTTPD_HOME}")
    message(STATUS "MICROHTTPD_HOME env is not set, setting it to /usr/local")
    set (MICROHTTPD_HOME ${_microhttpd_HOME})
  else("" MATCHES "$ENV{MICROHTTPD_HOME}")
    set (MICROHTTPD_HOME "$ENV{MICROHTTPD_HOME}")
  endif("" MATCHES "$ENV{MICROHTTPD_HOME}")
else( "${MICROHTTPD_HOME}" STREQUAL "")
  message(STATUS "MICROHTTPD_HOME is not empty: \"${MICROHTTPD_HOME}\"")
endif( "${MICROHTTPD_HOME}" STREQUAL "")
##

message(STATUS "Looking for microhttpd in ${MICROHTTPD_HOME}")

IF( NOT ${MICROHTTPD_HOME} STREQUAL "" )
    SET(_microhttpd_INCLUDE_SEARCH_DIRS ${MICROHTTPD_HOME}/include ${_microhttpd_INCLUDE_SEARCH_DIRS})
    SET(_microhttpd_LIBRARIES_SEARCH_DIRS ${MICROHTTPD_HOME}/lib ${_microhttpd_LIBRARIES_SEARCH_DIRS})
    SET(_microhttpd_HOME ${MICROHTTPD_HOME})
ENDIF( NOT ${MICROHTTPD_HOME} STREQUAL "" )

IF( NOT $ENV{MICROHTTPD_INCLUDEDIR} STREQUAL "" )
  SET(_microhttpd_INCLUDE_SEARCH_DIRS $ENV{MICROHTTPD_INCLUDEDIR} ${_microhttpd_INCLUDE_SEARCH_DIRS})
ENDIF( NOT $ENV{MICROHTTPD_INCLUDEDIR} STREQUAL "" )

IF( NOT $ENV{MICROHTTPD_LIBRARYDIR} STREQUAL "" )
  SET(_microhttpd_LIBRARIES_SEARCH_DIRS $ENV{MICROHTTPD_LIBRARYDIR} ${_microhttpd_LIBRARIES_SEARCH_DIRS})
ENDIF( NOT $ENV{MICROHTTPD_LIBRARYDIR} STREQUAL "" )

IF( MICROHTTPD_HOME )
  SET(_microhttpd_INCLUDE_SEARCH_DIRS ${MICROHTTPD_HOME}/include ${_microhttpd_INCLUDE_SEARCH_DIRS})
  SET(_microhttpd_LIBRARIES_SEARCH_DIRS ${MICROHTTPD_HOME}/lib ${_microhttpd_LIBRARIES_SEARCH_DIRS})
  SET(_microhttpd_HOME ${MICROHTTPD_HOME})
ENDIF( MICROHTTPD_HOME )

# find the include files
FIND_PATH(MICROHTTPD_INCLUDE_DIR microhttpd.h
   HINTS
     ${_microhttpd_INCLUDE_SEARCH_DIRS}
     ${PC_MICROHTTPD_INCLUDEDIR}
     ${PC_MICROHTTPD_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
)

# locate the library
IF(WIN32)
  SET(MICROHTTPD_LIBRARY_NAMES ${MICROHTTPD_LIBRARY_NAMES} libmicrohttpd.lib)
ELSE(WIN32)
  SET(MICROHTTPD_LIBRARY_NAMES ${MICROHTTPD_LIBRARY_NAMES} libmicrohttpd.a)
ENDIF(WIN32)
FIND_LIBRARY(MICROHTTPD_LIBRARY NAMES ${MICROHTTPD_LIBRARY_NAMES}
  HINTS
    ${_microhttpd_LIBRARIES_SEARCH_DIRS}
    ${PC_MICROHTTPD_LIBDIR}
    ${PC_MICROHTTPD_LIBRARY_DIRS}
)

# if the include and the program are found then we have it
IF(MICROHTTPD_INCLUDE_DIR AND MICROHTTPD_LIBRARY)
  SET(MICROHTTPD_FOUND "YES")
ENDIF(MICROHTTPD_INCLUDE_DIR AND MICROHTTPD_LIBRARY)

if( NOT WIN32)
  list(APPEND MICROHTTPD_LIBRARY "-lrt")
endif( NOT WIN32)

MARK_AS_ADVANCED(
  MICROHTTPD_FOUND
  MICROHTTPD_LIBRARY
  MICROHTTPD_INCLUDE_DIR
)
