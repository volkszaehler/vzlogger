# -*- mode: cmake; -*-
# - Try to find libsml include dirs and libraries
# Usage of this module as follows:
# This file defines:
# * SML_FOUND if protoc was found
# * SML_LIBRARY The lib to link to (currently only a static unix lib, not
# portable)
# * SML_INCLUDE The include directories for libsml.

message(STATUS "FindSml check")
IF (NOT WIN32)
  include(FindPkgConfig)
  if ( PKG_CONFIG_FOUND )

     pkg_check_modules (PC_SML sml>=1.0)

     set(SML_DEFINITIONS ${PC_SML_CFLAGS_OTHER})
  endif(PKG_CONFIG_FOUND)
endif (NOT WIN32)

#
# set defaults
SET(_sml_HOME "/usr/local")
SET(_sml_INCLUDE_SEARCH_DIRS
  ${CMAKE_INCLUDE_PATH}
  /usr/local/include
  /usr/include
  )

SET(_sml_LIBRARIES_SEARCH_DIRS
  ${CMAKE_LIBRARY_PATH}
  /usr/local/lib
  /usr/lib
  )

##
if( "${SML_HOME}" STREQUAL "")
  if("" MATCHES "$ENV{SML_HOME}")
    message(STATUS "SML_HOME env is not set, setting it to /usr/local")
    set (SML_HOME ${_sml_HOME})
  else("" MATCHES "$ENV{SML_HOME}")
    set (SML_HOME "$ENV{SML_HOME}")
  endif("" MATCHES "$ENV{SML_HOME}")
else( "${SML_HOME}" STREQUAL "")
  message(STATUS "SML_HOME is not empty: \"${SML_HOME}\"")
endif( "${SML_HOME}" STREQUAL "")
##

message(STATUS "Looking for sml in ${SML_HOME}")

IF( NOT ${SML_HOME} STREQUAL "" )
    SET(_sml_INCLUDE_SEARCH_DIRS ${SML_HOME}/include ${_sml_INCLUDE_SEARCH_DIRS})
    SET(_sml_LIBRARIES_SEARCH_DIRS ${SML_HOME}/lib ${_sml_LIBRARIES_SEARCH_DIRS})
    SET(_sml_HOME ${SML_HOME})
ENDIF( NOT ${SML_HOME} STREQUAL "" )

IF( NOT $ENV{SML_INCLUDEDIR} STREQUAL "" )
  SET(_sml_INCLUDE_SEARCH_DIRS $ENV{SML_INCLUDEDIR} ${_sml_INCLUDE_SEARCH_DIRS})
ENDIF( NOT $ENV{SML_INCLUDEDIR} STREQUAL "" )

IF( NOT $ENV{SML_LIBRARYDIR} STREQUAL "" )
  SET(_sml_LIBRARIES_SEARCH_DIRS $ENV{SML_LIBRARYDIR} ${_sml_LIBRARIES_SEARCH_DIRS})
ENDIF( NOT $ENV{SML_LIBRARYDIR} STREQUAL "" )

IF( SML_HOME )
  SET(_sml_INCLUDE_SEARCH_DIRS ${SML_HOME}/include ${_sml_INCLUDE_SEARCH_DIRS})
  SET(_sml_LIBRARIES_SEARCH_DIRS ${SML_HOME}/lib ${_sml_LIBRARIES_SEARCH_DIRS})
  SET(_sml_HOME ${SML_HOME})
ENDIF( SML_HOME )

# find the include files
FIND_PATH(SML_INCLUDE_DIR sml/sml_message.h
   HINTS
     ${_sml_INCLUDE_SEARCH_DIRS}
     ${PC_SML_INCLUDEDIR}
     ${PC_SML_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
)

# locate the library
IF(WIN32)
  SET(SML_LIBRARY_NAMES ${SML_LIBRARY_NAMES} libsml.lib)
ELSE(WIN32)
  SET(SML_LIBRARY_NAMES ${SML_LIBRARY_NAMES} libsml.a)
  SET(SML_LIBRARY_NAMES ${SML_LIBRARY_NAMES} libsml.so)
ENDIF(WIN32)
FIND_LIBRARY(SML_LIBRARY NAMES ${SML_LIBRARY_NAMES}
  HINTS
    ${_sml_LIBRARIES_SEARCH_DIRS}
    ${PC_SML_LIBDIR}
    ${PC_SML_LIBRARY_DIRS}
)

# if the include and the program are found then we have it
IF(SML_INCLUDE_DIR AND SML_LIBRARY)
  SET(SML_FOUND "YES")
ENDIF(SML_INCLUDE_DIR AND SML_LIBRARY)

if( NOT WIN32)
  list(APPEND SML_LIBRARY "-lrt")
endif( NOT WIN32)

MARK_AS_ADVANCED(
  SML_FOUND
  SML_LIBRARY
  SML_INCLUDE_DIR
)
