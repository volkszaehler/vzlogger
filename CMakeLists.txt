# -*- mode: cmake; -*-
project(vzlogger)
cmake_minimum_required(VERSION 2.6.2 FATAL_ERROR)
if(POLICY CMP0011)
  cmake_policy(SET CMP0011 NEW)
endif(POLICY CMP0011)

set(VZLOGGER_MAJOR_VERSION 0)
set(VZLOGGER_MINOR_VERSION 3)
set(VZLOGGER_SUB_VERSION   4)
set(VERSION_SEP "-")
set(VZLOGGER_SHORT_VERSION "${VZLOGGER_MAJOR_VERSION}${VERSION_SEP}${VZLOGGER_MINOR_VERSION}")
set(VZLOGGER_VERSION "${VZLOGGER_SHORT_VERSION}${VERSION_SEP}${VZLOGGER_SUB_VERSION}")
set(VZLOGGER_RPM_VERSION "${VZLOGGER_MAJOR_VERSION}.${VZLOGGER_MINOR_VERSION}.${VZLOGGER_SUB_VERSION}")
set(VERSION_DATE "2012-02-20")
set(PACKAGE_STRING "${PROJECT_NAME} ${VZLOGGER_RPM_VERSION}")
set(VERSION "${VZLOGGER_RPM_VERSION}")

###########################################################
# Where are the additional libraries installed? Note: provide includes
# path here, subsequent checks will resolve everything else
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/modules")
include (UseCodeCoverage)

message("Compiling for target '${TARGET}'")
if( TARGET )
  if( ${TARGET} STREQUAL "ar71xx")
    include ( OpenWRT )
    message("Compiling for ar71xx")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(LINK_FLAGS_RELEASE "-static")
  endif( ${TARGET} STREQUAL "ar71xx")
else( TARGET )
  include ( CompilerFlags )
endif( TARGET )

include ( ConfigureMacro )

# options
OPTION(ENABLE_SML
  "enable support for smart messaging language (def=yes)])"
  On)
OPTION(ENABLE_LOCAL
  "enable support for local HTTPd (def=yes)]"
  On)
OPTION(WITH_READER
  "compile reader to for testing your meters (def=yes)])"
  On)

# find dependencies
# libsml
if( ENABLE_SML )
  include(FindSML)
  if(SML_FOUND)
    set(SML_SUPPORT 1)
  endif(SML_FOUND)
endif( ENABLE_SML )

if(ENABLE_LOCAL)
  include(FindMicroHttpd)
  if(MICROHTTPD_FOUND)
    set(MICROHTTPD_SUPPORT 1)
  endif(MICROHTTPD_FOUND)
endif(ENABLE_LOCAL)

IF(NOT WIN32)
  include(FindThreads)
ENDIF(NOT WIN32)

# libjson
include(FindJson)
if( NOT JSON_FOUND)
  message(FATAL_ERROR "libjson ist required")
endif( NOT JSON_FOUND)
include_directories(${JSON_INCLUDE_DIR})

# check for curl
IF(WIN32)
  add_definitions(-DCURL_STATICLIB)
  include(FindCURL_WIN32)
else(WIN32)
  #  add_definitions(-DCURL_STATICLIB)
  include(FindCurl)
  include(FindGnutls)
endif(WIN32)

find_library(LIBUUID uuid)

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR})

add_definitions(-DHAVE_CONFIG_HPP)
myconfigure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.hpp
  )

add_subdirectory(src)

# enable unit testing
include(CTest)
enable_testing()

### print some output for the user
message("")
message("        ***** Configuration parameters *****")
message("             prefix: ${CMAKE_INSTALL_PREFIX}")
message("             json: -L${JSON_LIBRARY} -I${JSON_INCLUDE_DIR}")
message("             sml:  -L${SML_LIBRARY} -I${SML_INCLUDE_DIR}")
message("             microhttpd: -L${MICROHTTPD_LIBRARY} -I${MICROHTTPD_INCLUDE_DIR}")

# add some files to the installation target
INSTALL(FILES README INSTALL COPYING DESTINATION
  share/doc/vzlogger-${V_MAJOR}-${V_MINOR}
)

# CPACK packaging
INCLUDE(InstallRequiredSystemLibraries)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS On)
#set(CPACK_SOURCE_GENERATOR "TGZ;TBZ2;PKG")
set(CPACK_SOURCE_GENERATOR "TGZ;TBZ2")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "volkszaehler.org ist ein freies Smart Meter (hier: intelligenter Stromzähler) im Selbstbau.")
set(CPACK_PACKAGE_VENDOR              "Steffen Vogel")
set(CPACK_PACKAGE_CONTACT             "Steffen Vogel <info@steffenvogel.de>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER   "Steffen Vogel <info@steffenvogel.de>")
set(CPACK_PACKAGE_DESCRIPTION_FILE    "${CMAKE_CURRENT_SOURCE_DIR}/README")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION_MAJOR ${VZLOGGER_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${VZLOGGER_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${VZLOGGER_SUB_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}-${VZLOGGER_MAJOR_VERSION}.${VZLOGGER_MINOR_VERSION}")

set(CPACK_SOURCE_IGNORE_FILES "/CVS/;/.svn/;.swp$;.#;/#;.*~;cscope.*;build.*")

IF(WIN32 AND NOT UNIX)
  set(CPACK_GENERATOR "NSIS")
  set(CPACK_NSIS_PACKAGE_NAME "Gridlicense Demo Tools")
  # set(CPACK_PACKAGE_EXECUTABLES "gui4" "Grid License Demo Frontend")
  set(CPACK_PACKAGE_DEFAULT_LOCATION ${INSTALL_DIR})

  # sure there is at least one set of four (4) backlasshes.
  set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\vzlogger.exe")
  set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} VZlogger")
  set(CPACK_NSIS_HELP_LINK    "http://volkszaehler.org/")
  set(CPACK_NSIS_URL_INFO_ABOUT "http://wiki.volkszaehler.org/software/controller/vzlogger")
  set(CPACK_NSIS_CONTACT "Steffen Vogel <info@steffenvogel.de>")
  set(CPACK_NSIS_MODIFY_PATH ON)
  # set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CMAKE_PROJECT_NAME}-${EWS_VERSION}")
  # set(CPACK_NSIS_MENU_LINKS "${EWS_WEBSITE}" "Homepage for ${EWS_APP_NAME}")
  set(CPACK_NSIS_DEFINES "  !define MUI_WELCOMEPAGE_TITLE_3LINES 1
    !define MUI_FINISHPAGE_TITLE_3LINES 1")
else(WIN32 AND NOT UNIX)
  #set(CPACK_SET_DESTDIR On)
  set(CPACK_SOURCE_STRIP_FILES "")
  set(CPACK_GENERATOR "TGZ;TBZ2;TZ;DEB;RPM")
  set(CPACK_RPM_PACKAGE_RELOCATABLE 1)
  set(CPACK_SYSTEM_NAME ${CMAKE_HOST_SYSTEM_PROCESSOR})
endif(WIN32 AND NOT UNIX)
include(CPack)

