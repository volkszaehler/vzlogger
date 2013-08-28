# -*- mode: cmake; -*-
# locates the gnutls library
# This file defines:
# * GNUTLS_FOUND if gnutls was found
# * GNUTLS_LIBRARY The lib to link to (currently only a static unix lib) 
# * GNUTLS_INCLUDE_DIR

if (NOT GNUTLS_FIND_QUIETLY)
  message(STATUS "FindGnuTls check")
endif (NOT GNUTLS_FIND_QUIETLY)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  include(FindPackageHandleStandardArgs)

  if (NOT WIN32)
    include(FindPkgConfig)
    if ( PKG_CONFIG_FOUND OR NOT ${PKG_CONFIG_EXECUTABLE} STREQUAL "PKG_CONFIG_EXECUTABLE-NOTFOUND")

      pkg_check_modules (PC_GNUTLS gnutls>=2.8)

      set(GNUTLS_DEFINITIONS ${PC_GNUTLS_CFLAGS_OTHER})
      message(STATUS "==> '${PC_GNUTLS_CFLAGS_OTHER}'")
    else(PKG_CONFIG_FOUND OR NOT ${PKG_CONFIG_EXECUTABLE} STREQUAL "PKG_CONFIG_EXECUTABLE-NOTFOUND")
      message(STATUS "==> N '${PC_GNUTLS_CFLAGS_OTHER}'")
    endif(PKG_CONFIG_FOUND OR NOT ${PKG_CONFIG_EXECUTABLE} STREQUAL "PKG_CONFIG_EXECUTABLE-NOTFOUND")
  endif (NOT WIN32)

  #
  # set defaults
  set(_gnutls_HOME "/usr/local")
  set(_gnutls_INCLUDE_SEARCH_DIRS
    ${CMAKE_INCLUDE_PATH}
    /usr/local/include
    /usr/include
    )

  set(_gnutls_LIBRARIES_SEARCH_DIRS
    ${CMAKE_LIBRARY_PATH}
    /usr/local/lib
    /usr/lib
    )

  ##
  if( "${GNUTLS_HOME}" STREQUAL "")
    if("" MATCHES "$ENV{GNUTLS_HOME}")
      message(STATUS "GNUTLS_HOME env is not set, setting it to /usr/local")
      set (GNUTLS_HOME ${_gnutls_HOME})
    else("" MATCHES "$ENV{GNUTLS_HOME}")
      set (GNUTLS_HOME "$ENV{GNUTLS_HOME}")
    endif("" MATCHES "$ENV{GNUTLS_HOME}")
  else( "${GNUTLS_HOME}" STREQUAL "")
    message(STATUS "GNUTLS_HOME is not empty: \"${GNUTLS_HOME}\"")
  endif( "${GNUTLS_HOME}" STREQUAL "")
  ##

  message(STATUS "Looking for gnutls in ${GNUTLS_HOME}")

  if( NOT ${GNUTLS_HOME} STREQUAL "" )
    set(_gnutls_INCLUDE_SEARCH_DIRS ${GNUTLS_HOME}/include ${_gnutls_INCLUDE_SEARCH_DIRS})
    set(_gnutls_LIBRARIES_SEARCH_DIRS ${GNUTLS_HOME}/lib ${_gnutls_LIBRARIES_SEARCH_DIRS})
    set(_gnutls_HOME ${GNUTLS_HOME})
  endif( NOT ${GNUTLS_HOME} STREQUAL "" )

  if( NOT $ENV{GNUTLS_INCLUDEDIR} STREQUAL "" )
    set(_gnutls_INCLUDE_SEARCH_DIRS $ENV{GNUTLS_INCLUDEDIR} ${_gnutls_INCLUDE_SEARCH_DIRS})
  endif( NOT $ENV{GNUTLS_INCLUDEDIR} STREQUAL "" )

  if( NOT $ENV{GNUTLS_LIBRARYDIR} STREQUAL "" )
    set(_gnutls_LIBRARIES_SEARCH_DIRS $ENV{GNUTLS_LIBRARYDIR} ${_gnutls_LIBRARIES_SEARCH_DIRS})
  endif( NOT $ENV{GNUTLS_LIBRARYDIR} STREQUAL "" )

  if( GNUTLS_HOME )
    set(_gnutls_INCLUDE_SEARCH_DIRS ${GNUTLS_HOME}/include ${_gnutls_INCLUDE_SEARCH_DIRS})
    set(_gnutls_LIBRARIES_SEARCH_DIRS ${GNUTLS_HOME}/lib ${_gnutls_LIBRARIES_SEARCH_DIRS})
    set(_gnutls_HOME ${GNUTLS_HOME})
  endif( GNUTLS_HOME )

  # find the include files
  find_path(GNUTLS_INCLUDE_DIR gnutls/gnutls.h
    HINTS
    ${_gnutls_INCLUDE_SEARCH_DIRS}
    ${PC_GNUTLS_INCLUDEDIR}
    ${PC_GNUTLS_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
    )

  # locate the library
  if(WIN32)
    set(GNUTLS_LIBRARY_NAMES ${GNUTLS_LIBRARY_NAMES} libgnutls.lib)
  else(WIN32)
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      # On MacOS
      set(GNUTLS_LIBRARY_NAMES ${GNUTLS_LIBRARY_NAMES} libgnutls.dylib)
    else(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set(GNUTLS_LIBRARY_NAMES ${GNUTLS_LIBRARY_NAMES} libgnutls.a)
    endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  endif(WIN32)

  if( PC_GNUTLS_STATIC_LIBRARIES )
    foreach(lib ${PC_GNUTLS_STATIC_LIBRARIES})
      string(TOUPPER ${lib} _NAME_UPPER)

      find_library(GNUTLS_${_NAME_UPPER}_LIBRARY NAMES "lib${lib}.a"
	HINTS
	${_gnutls_LIBRARIES_SEARCH_DIRS}
	${PC_GNUTLS_LIBDIR}
	${PC_GNUTLS_LIBRARY_DIRS}
	)
      #list(APPEND GNUTLS_LIBRARIES ${_dummy})
    endforeach()
    set(_GNUTLS_LIBRARIES "")
    foreach(lib ${PC_GNUTLS_STATIC_LIBRARIES} )
      string(TOUPPER ${lib} _NAME_UPPER)
      if( NOT ${GNUTLS_${_NAME_UPPER}_LIBRARY} STREQUAL "GNUTLS_${_NAME_UPPER}_LIBRARY-NOTFOUND" )
	set(_GNUTLS_LIBRARIES ${_GNUTLS_LIBRARIES} ${GNUTLS_${_NAME_UPPER}_LIBRARY})
      else( NOT ${GNUTLS_${_NAME_UPPER}_LIBRARY} STREQUAL "GNUTLS_${_NAME_UPPER}_LIBRARY-NOTFOUND" )
	set(_GNUTLS_LIBRARIES ${_GNUTLS_LIBRARIES} -l${lib})
      endif( NOT ${GNUTLS_${_NAME_UPPER}_LIBRARY} STREQUAL "GNUTLS_${_NAME_UPPER}_LIBRARY-NOTFOUND" )
    endforeach()
    set(GNUTLS_LIBRARIES ${_GNUTLS_LIBRARIES} CACHE FILEPATH "")
  endif( PC_GNUTLS_STATIC_LIBRARIES )
  #else( PC_GNUTLS_STATIC_LIBRARIES )
    find_library(GNUTLS_LIBRARY NAMES ${GNUTLS_LIBRARY_NAMES}
      HINTS
      ${_gnutls_LIBRARIES_SEARCH_DIRS}
      ${PC_GNUTLS_LIBDIR}
      ${PC_GNUTLS_LIBRARY_DIRS}
      )
  #endif( PC_GNUTLS_STATIC_LIBRARIES )

  message("==> GNUTLS_LIBRARIES='${GNUTLS_LIBRARIES}'")
  # On Linux
  find_library (GNUTLS_SHARED_LIBRARY
    NAMES libgnutls.so
    HINTS ${GNUTLS_HOME} ENV GNUTLS_HOME
    PATH_SUFFIXES lib
    )

find_package_handle_standard_args(GNUTLS   DEFAULT_MSG GNUTLS_LIBRARIES GNUTLS_INCLUDE_DIR)

else(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  set(GNUTLS_FOUND true)
  set(GNUTLS_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/gnutls ${CMAKE_BINARY_DIR}/gnutls)
  set(GNUTLS_LIBRARY_DIR "")
  set(GNUTLS_LIBRARY gnutls)
endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})

