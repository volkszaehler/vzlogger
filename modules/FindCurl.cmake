# -*- mode: cmake; -*-
# locates the curl library
# This file defines:
# * CURL_FOUND if curl was found
# * CURL_LIBRARY The lib to link to (currently only a static unix lib) 
# * CURL_INCLUDE_DIR

if (NOT CURL_FIND_QUIETLY)
  message(STATUS "FindCURL check")
endif (NOT CURL_FIND_QUIETLY)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  include(FindPackageHandleStandardArgs)

  if (NOT WIN32)
    include(FindPkgConfig)
    if ( PKG_CONFIG_FOUND OR NOT ${PKG_CONFIG_EXECUTABLE} STREQUAL "PKG_CONFIG_EXECUTABLE-NOTFOUND")

      pkg_check_modules (PC_CURL libcurl>=7.19)

      set(CURL_DEFINITIONS ${PC_CURL_CFLAGS_OTHER})
      message(STATUS "==> '${PC_CURL_CFLAGS_OTHER}'")
    else(PKG_CONFIG_FOUND OR NOT ${PKG_CONFIG_EXECUTABLE} STREQUAL "PKG_CONFIG_EXECUTABLE-NOTFOUND")
      message(STATUS "==> N '${PC_CURL_CFLAGS_OTHER}'")
    endif(PKG_CONFIG_FOUND OR NOT ${PKG_CONFIG_EXECUTABLE} STREQUAL "PKG_CONFIG_EXECUTABLE-NOTFOUND")
  endif (NOT WIN32)

  #
  # set defaults
  set(_curl_HOME "/usr/local")
  set(_curl_INCLUDE_SEARCH_DIRS
    ${CMAKE_INCLUDE_PATH}
    /usr/local/include
    /usr/include
    )

  set(_curl_LIBRARIES_SEARCH_DIRS
    ${CMAKE_LIBRARY_PATH}
    /usr/local/lib
    /usr/lib
    )

  ##
  if( "${CURL_HOME}" STREQUAL "")
    if("" MATCHES "$ENV{CURL_HOME}")
      message(STATUS "CURL_HOME env is not set, setting it to /usr/local")
      set (CURL_HOME ${_curl_HOME})
    else("" MATCHES "$ENV{CURL_HOME}")
      set (CURL_HOME "$ENV{CURL_HOME}")
    endif("" MATCHES "$ENV{CURL_HOME}")
  else( "${CURL_HOME}" STREQUAL "")
    message(STATUS "CURL_HOME is not empty: \"${CURL_HOME}\"")
  endif( "${CURL_HOME}" STREQUAL "")
  ##

  message(STATUS "Looking for curl in ${CURL_HOME}")

  if( NOT ${CURL_HOME} STREQUAL "" )
    set(_curl_INCLUDE_SEARCH_DIRS ${CURL_HOME}/include ${_curl_INCLUDE_SEARCH_DIRS})
    set(_curl_LIBRARIES_SEARCH_DIRS ${CURL_HOME}/lib ${_curl_LIBRARIES_SEARCH_DIRS})
    set(_curl_HOME ${CURL_HOME})
  endif( NOT ${CURL_HOME} STREQUAL "" )

  if( NOT $ENV{CURL_INCLUDEDIR} STREQUAL "" )
    set(_curl_INCLUDE_SEARCH_DIRS $ENV{CURL_INCLUDEDIR} ${_curl_INCLUDE_SEARCH_DIRS})
  endif( NOT $ENV{CURL_INCLUDEDIR} STREQUAL "" )

  if( NOT $ENV{CURL_LIBRARYDIR} STREQUAL "" )
    set(_curl_LIBRARIES_SEARCH_DIRS $ENV{CURL_LIBRARYDIR} ${_curl_LIBRARIES_SEARCH_DIRS})
  endif( NOT $ENV{CURL_LIBRARYDIR} STREQUAL "" )

  if( CURL_HOME )
    set(_curl_INCLUDE_SEARCH_DIRS ${CURL_HOME}/include ${_curl_INCLUDE_SEARCH_DIRS})
    set(_curl_LIBRARIES_SEARCH_DIRS ${CURL_HOME}/lib ${_curl_LIBRARIES_SEARCH_DIRS})
    set(_curl_HOME ${CURL_HOME})
  endif( CURL_HOME )

  # find the include files
  find_path(CURL_INCLUDE_DIR curl/curl.h
    HINTS
    ${_curl_INCLUDE_SEARCH_DIRS}
    ${PC_CURL_INCLUDEDIR}
    ${PC_CURL_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
    )

  # locate the library
  if(WIN32)
    set(CURL_LIBRARY_NAMES ${CURL_LIBRARY_NAMES} libcurl.lib)
  else(WIN32)
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      # On MacOS
      set(CURL_LIBRARY_NAMES ${CURL_LIBRARY_NAMES} libcurl.dylib)
    else(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set(CURL_LIBRARY_NAMES ${CURL_LIBRARY_NAMES} libcurl.a)
    endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  endif(WIN32)

  if( PC_CURL_STATIC_LIBRARIES )
    foreach(lib ${PC_CURL_STATIC_LIBRARIES})
      string(TOUPPER ${lib} _NAME_UPPER)

      find_library(CURL_${_NAME_UPPER}_LIBRARY NAMES "lib${lib}.a"
	HINTS
	${_curl_LIBRARIES_SEARCH_DIRS}
	${PC_CURL_LIBDIR}
	${PC_CURL_LIBRARY_DIRS}
	)
      #list(APPEND CURL_LIBRARIES ${_dummy})
    endforeach()
    set(_CURL_LIBRARIES "")
    foreach(lib ${PC_CURL_STATIC_LIBRARIES})
      string(TOUPPER ${lib} _NAME_UPPER)
      if( NOT ${CURL_${_NAME_UPPER}_LIBRARY} STREQUAL "CURL_${_NAME_UPPER}_LIBRARY-NOTFOUND" )
	set(_CURL_LIBRARIES ${_CURL_LIBRARIES} ${CURL_${_NAME_UPPER}_LIBRARY})
      else( NOT ${CURL_${_NAME_UPPER}_LIBRARY} STREQUAL "CURL_${_NAME_UPPER}_LIBRARY-NOTFOUND" )
	set(_CURL_LIBRARIES ${_CURL_LIBRARIES} -l${lib})
      endif( NOT ${CURL_${_NAME_UPPER}_LIBRARY} STREQUAL "CURL_${_NAME_UPPER}_LIBRARY-NOTFOUND" )
    endforeach()
    set(CURL_LIBRARIES ${_CURL_LIBRARIES} CACHE FILEPATH "")
  endif( PC_CURL_STATIC_LIBRARIES )
  #else( PC_CURL_STATIC_LIBRARIES )
    find_library(CURL_LIBRARY NAMES ${CURL_LIBRARY_NAMES}
      HINTS
      ${_curl_LIBRARIES_SEARCH_DIRS}
      ${PC_CURL_LIBDIR}
      ${PC_CURL_LIBRARY_DIRS}
      )
  #endif( PC_CURL_STATIC_LIBRARIES )

  message("==> CURL_LIBRARIES='${CURL_LIBRARIES}'")
  # On Linux
  find_library (CURL_SHARED_LIBRARY
    NAMES libcurl.so
    HINTS ${CURL_HOME} ENV CURL_HOME
    PATH_SUFFIXES lib
    )


#  if (CURL_INCLUDE_DIR AND CURL_LIBRARY)
#    set (CURL_FOUND TRUE)
#    if (NOT CURL_FIND_QUIETLY)
#      message (STATUS "Found curl headers in ${CURL_INCLUDE_DIR} and libraries ${CURL_LIBRARY}")
#    endif (NOT CURL_FIND_QUIETLY)
#  else (CURL_INCLUDE_DIR AND CURL_LIBRARY)
#    if (CURL_FIND_REQUIRED)
#      message (FATAL_ERROR "curl could not be found!")
#    endif (CURL_FIND_REQUIRED)
#  endif (CURL_INCLUDE_DIR AND CURL_LIBRARY)

find_package_handle_standard_args(CURL   DEFAULT_MSG CURL_LIBRARIES CURL_INCLUDE_DIR)

else(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  set(CURL_FOUND true)
  set(CURL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/curl ${CMAKE_BINARY_DIR}/curl)
  set(CURL_LIBRARY_DIR "")
  set(CURL_LIBRARY curl)
endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})

