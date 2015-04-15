# -*- mode: cmake; -*-
# - Try to find libleptonica include dirs and libraries
# Usage of this module as follows:
# This file defines:
# * LEPTONICA_FOUND if protoc was found
# * LEPTONICA_LIBRARIES The lib to link to (currently only a static unix lib, not
# portable)
# * LEPTONICA_INCLUDE_DIRs The include directories for libleptonica.

message(STATUS "FindLeptonica check")
IF (NOT WIN32)
  include(FindPkgConfig)
  if ( PKG_CONFIG_FOUND )

     pkg_check_modules (LEPTONICA lept>=1.71)

  endif(PKG_CONFIG_FOUND)
endif (NOT WIN32)

#
# set defaults -> no defaults. leptonica ships with lept.pc

message(STATUS "Looking for leptonica in ${LEPTONICA_INCLUDE_DIRS}")

# find the include files
FIND_PATH(LEPTONICA_INCLUDE_DIR allheaders.h
   HINTS
     ${LEPTONICA_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
)

# locate the library
FIND_LIBRARY(LEPTONICA_LIBRARY NAMES ${LEPTONICA_LIBRARIES}
  HINTS
    ${PC_LEPTONICA_LIBRARY_DIRS}
)

# if the include and the program are found then we have it
# we don't use the ..._INCLUDE_DIR and _LIBRARY but the _INCLUDE_DIRS and _LIBRARIES
IF(LEPTONICA_INCLUDE_DIR AND LEPTONICA_LIBRARY)
  SET(LEPTONICA_FOUND "YES")
else ()
  message(STATUS "Leptonica installation seems to be corrupt.")
  SET(LEPTONICA_FOUND "NO")
ENDIF(LEPTONICA_INCLUDE_DIR AND LEPTONICA_LIBRARY)

MARK_AS_ADVANCED(
  LEPTONICA_FOUND
)
