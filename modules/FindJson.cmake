# -*- mode: cmake; -*-
# - Try to find libjson include dirs and libraries
# Usage of this module as follows:
# This file defines:
# * JSON_FOUND if protoc was found
# * JSON_LIBRARY The lib to link to (currently only a static unix lib, not
# portable)
# * JSON_INCLUDE The include directories for libjson.

message(STATUS "FindJson check")
IF (NOT WIN32)
  include(FindPkgConfig)
  if ( PKG_CONFIG_FOUND )

     pkg_check_modules (PC_JSON json-c>=0.12)

     set(JSON_DEFINITIONS ${PC_JSON_CFLAGS_OTHER})
  endif(PKG_CONFIG_FOUND)
endif (NOT WIN32)

#
# set defaults
SET(_json_HOME "/usr/local")
SET(_json_INCLUDE_SEARCH_DIRS
  ${CMAKE_INCLUDE_PATH}
  /usr/local/include
  /usr/include
  )

SET(_json_LIBRARIES_SEARCH_DIRS
  ${CMAKE_LIBRARY_PATH}
  /usr/local/lib
  /usr/lib
  )

##
if( "${JSON_HOME}" STREQUAL "")
  if("" MATCHES "$ENV{JSON_HOME}")
    message(STATUS "JSON_HOME env is not set, setting it to /usr/local")
    set (JSON_HOME ${_json_HOME})
  else("" MATCHES "$ENV{JSON_HOME}")
    set (JSON_HOME "$ENV{JSON_HOME}")
  endif("" MATCHES "$ENV{JSON_HOME}")
else( "${JSON_HOME}" STREQUAL "")
  message(STATUS "JSON_HOME is not empty: \"${JSON_HOME}\"")
endif( "${JSON_HOME}" STREQUAL "")
##

message(STATUS "Looking for json in ${JSON_HOME}")

IF( NOT ${JSON_HOME} STREQUAL "" )
    SET(_json_INCLUDE_SEARCH_DIRS ${JSON_HOME}/include ${_json_INCLUDE_SEARCH_DIRS})
    SET(_json_LIBRARIES_SEARCH_DIRS ${JSON_HOME}/lib ${_json_LIBRARIES_SEARCH_DIRS})
    SET(_json_HOME ${JSON_HOME})
ENDIF( NOT ${JSON_HOME} STREQUAL "" )

IF( NOT $ENV{JSON_INCLUDEDIR} STREQUAL "" )
  SET(_json_INCLUDE_SEARCH_DIRS $ENV{JSON_INCLUDEDIR} ${_json_INCLUDE_SEARCH_DIRS})
ENDIF( NOT $ENV{JSON_INCLUDEDIR} STREQUAL "" )

IF( NOT $ENV{JSON_LIBRARYDIR} STREQUAL "" )
  SET(_json_LIBRARIES_SEARCH_DIRS $ENV{JSON_LIBRARYDIR} ${_json_LIBRARIES_SEARCH_DIRS})
ENDIF( NOT $ENV{JSON_LIBRARYDIR} STREQUAL "" )

IF( JSON_HOME )
  SET(_json_INCLUDE_SEARCH_DIRS ${JSON_HOME}/include ${_json_INCLUDE_SEARCH_DIRS})
  SET(_json_LIBRARIES_SEARCH_DIRS ${JSON_HOME}/lib ${_json_LIBRARIES_SEARCH_DIRS})
  SET(_json_HOME ${JSON_HOME})
ENDIF( JSON_HOME )

message("Json-c search: '${_json_INCLUDE_SEARCH_DIRS}' ${CMAKE_INCLUDE_PATH}")
# find the include files
FIND_PATH(JSON-C_INCLUDE_DIR json-c/json.h
   HINTS
     ${_json_INCLUDE_SEARCH_DIRS}
     ${PC_JSON_INCLUDEDIR}
     ${PC_JSON_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
)

# locate the library
IF(WIN32)
  SET(JSON_LIBRARY_NAMES ${JSON_LIBRARY_NAMES} libjson.lib)
ELSE(WIN32)
  SET(JSON_LIBRARY_NAMES ${JSON_LIBRARY_NAMES} libjson-c.so)
  SET(JSON_LIBRARY_NAMES ${JSON_LIBRARY_NAMES} libjson-c.a)
ENDIF(WIN32)
FIND_LIBRARY(JSON_LIBRARY NAMES ${JSON_LIBRARY_NAMES}
  HINTS
    ${_json_LIBRARIES_SEARCH_DIRS}
    ${PC_JSON_LIBDIR}
    ${PC_JSON_LIBRARY_DIRS}
)

# if the include and the program are found then we have it
IF(JSON-C_INCLUDE_DIR AND JSON_LIBRARY)
  SET(JSON_FOUND "YES")
  message("Json-c found: '${JSON-C_INCLUDE_DIR}'")
  SET(JSON_INCLUDE_DIR ${JSON-C_INCLUDE_DIR})
ENDIF(JSON-C_INCLUDE_DIR AND JSON_LIBRARY)

if( NOT WIN32)
  list(APPEND JSON_LIBRARY "-lrt")
endif( NOT WIN32)

MARK_AS_ADVANCED(
  JSON_FOUND
  JSON_LIBRARY
  JSON_INCLUDE_DIR
)
