# -*- mode: cmake; -*-
# 
# ** Remember to change anything in the format of <SOME TEXT> to whatever should go there. 
# ** You'll have to look to find the right directories and binaries.
# 
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

message(STATUS "Setup Cross Compiling Environement for openwrt.")

# Search for openwrt installation
if( "${OPENWRT_HOME}" STREQUAL "")
  if("" MATCHES "$ENV{OPENWRT_HOME}")
    message(STATUS "OPENWRT_HOME env is not set, setting it to /usr/local")
    set (OPENWRT_HOME ${_curl_HOME})
  else("" MATCHES "$ENV{OPENWRT_HOME}")
    set (OPENWRT_HOME "$ENV{OPENWRT_HOME}")
  endif("" MATCHES "$ENV{OPENWRT_HOME}")
else( "${OPENWRT_HOME}" STREQUAL "")
  message(STATUS "OPENWRT_HOME is not empty: \"${OPENWRT_HOME}\"")
endif( "${OPENWRT_HOME}" STREQUAL "")

##
find_file(_openwrt_configuration
  NAMES .config
  HINTS ${OPENWRT_HOME}
  DOC "find OpenWRT configuration file"
  )

if( ${_openwrt_configuration} STREQUAL "_openwrt_configuration-NOTFOUND" )
  message(FATAL_ERROR  "OpenWRT cross-compiling environement not found.")
endif( ${_openwrt_configuration} STREQUAL "_openwrt_configuration-NOTFOUND" )
#include(${_openwrt_configuration})

# read configuragtion file
file(READ ${_openwrt_configuration} _openwrt_config )
#string(REGEX REPLACE "#[A-Za-z0-9 =_-]+\n" "" _openwrt_config_passed ${_openwrt_config})
string(REGEX MATCH "CONFIG_ARCH=\"[a-zA-Z0-9]+\"" _dummy ${_openwrt_config})
string(REGEX REPLACE "CONFIG_ARCH=\"(.*)\"" "\\1" openwrt_arch ${_dummy})

string(REGEX MATCH "CONFIG_TARGET_BOARD=\"([a-zA-Z0-9]+)\"" _dummy ${_openwrt_config})
string(REGEX REPLACE "CONFIG_TARGET_BOARD=\"(.*)\"" "\\1" openwrt_target_board ${_dummy})

string(REGEX MATCH "CONFIG_LIBC=\"[a-zA-Z0-9]+\"" _dummy ${_openwrt_config})
string(REGEX REPLACE "CONFIG_LIBC=\"(.*)\"" "\\1" openwrt_libc ${_dummy})

string(REGEX MATCH "CONFIG_LIBC_VERSION=\"[a-zA-Z0-9.]+\"" _dummy ${_openwrt_config})
string(REGEX REPLACE "CONFIG_LIBC_VERSION=\"(.*)\"" "\\1" openwrt_libc_version ${_dummy})

string(REGEX MATCH "CONFIG_GCC_VERSION=\"[a-zA-Z0-9.+]+\"" _dummy ${_openwrt_config})
string(REGEX REPLACE "CONFIG_GCC_VERSION=\"(.*)\"" "\\1" openwrt_gcc_version ${_dummy})

message(STATUS "ARCH         '${openwrt_arch}'")
message(STATUS "LIBC         '${openwrt_libc}'")
message(STATUS "LIBC_version '${openwrt_libc_version}'")
message(STATUS "BOARD'       '${openwrt_target_board}'")
message(STATUS "GCC'         '${openwrt_gcc_version}'")

#CONFIG_GCC_VERSION="4.3.3+cs"

set(openwrt_system       "linux")

set(openwrt_toolchain "toolchain-${openwrt_arch}_r2_gcc-${openwrt_gcc_version}_${openwrt_libc}-${openwrt_libc_version}")
set(openwrt_target    "target-${openwrt_arch}_r2_${openwrt_libc}-${openwrt_libc_version}")
set(_openwrt_c_compiler  "${openwrt_arch}-openwrt-${openwrt_system}-gcc")
set(_openwrt_cxx_compiler  "${openwrt_arch}-openwrt-${openwrt_system}-g++")

# set(OPENWRT_HOME /homes/krueger/Project/MySmartGrid/backfire_ubuntu/build/openwrt)
set(OPENWRT_STAGING_DIR ${OPENWRT_HOME}/staging_dir)
set(OPENWRT_STAGING_DIR_HOST ${OPENWRT_STAGING_DIR}/host)
set(OPENWRT_TOOLCHAIN_DIR ${OPENWRT_STAGING_DIR}/${openwrt_toolchain})
set(OPENWRT_TARGET_DIR    ${OPENWRT_STAGING_DIR}/${openwrt_target})


set(ENV{STAGING_DIR}     ${OPENWRT_STAGING_DIR})
set(ENV{PKG_CONFIG_PATH} ${OPENWRT_TARGET_DIR}/usr/lib/pkgconfig)
message("export STAGING_DIR=${OPENWRT_STAGING_DIR}")
message("export STAGING_DIR=$ENV{STAGING_DIR}")
message("export PKG_CONFIG_PATH=$ENV{PKG_CONFIG_PATH}")

#compiler
find_program(openwrt_c_compiler
  NAMES ${_openwrt_c_compiler}
  HINTS ${OPENWRT_TOOLCHAIN_DIR}
  PATH_SUFFIXES bin
  DOC "find gcc compiler"
  NO_DEFAULT_PATH
  NO_CMAKE_ENVIRONMENT_PATH
  NO_CMAKE_PATH
  NO_SYSTEM_ENVIRONMENT_PATH
  NO_CMAKE_SYSTEM_PATH
)
if( ${openwrt_c_compiler} STREQUAL "openwrt_c_compiler-NOTFOUND" )
  message(FATAL_ERROR  "OpenWRT cross c-compiler ${openwrt_compiler} not found.")
endif( ${openwrt_c_compiler} STREQUAL "openwrt_c_compiler-NOTFOUND" )

find_program(openwrt_cxx_compiler
  NAMES ${_openwrt_cxx_compiler}
  HINTS ${OPENWRT_TOOLCHAIN_DIR}
  PATH_SUFFIXES bin
  DOC "find gcc compiler"
  NO_DEFAULT_PATH
  NO_CMAKE_ENVIRONMENT_PATH
  NO_CMAKE_PATH
  NO_SYSTEM_ENVIRONMENT_PATH
  NO_CMAKE_SYSTEM_PATH
)
if( ${openwrt_cxx_compiler} STREQUAL "openwrt_cxx_compiler-NOTFOUND" )
  message(FATAL_ERROR  "OpenWRT cross c++-compiler ${openwrt_compiler} not found.")
endif( ${openwrt_cxx_compiler} STREQUAL "openwrt_cxx_compiler-NOTFOUND" )

SET(CMAKE_C_COMPILER   ${openwrt_c_compiler})
SET(CMAKE_CXX_COMPILER ${openwrt_cxx_compiler})

#SET(CMAKE_C_FLAGS "-ffreestanding -fno-exceptions -fno-stack-protector")
#SET(CMAKE_CXX_FLAGS "-ffreestanding -fno-exceptions -fno-stack-protector")
SET(CMAKE_C_FLAGS "-ffreestanding  -fno-stack-protector")
SET(CMAKE_CXX_FLAGS "-ffreestanding  -fno-stack-protector")

SET(CMAKE_FIND_ROOT_PATH ${OPENWRT_TARGET_DIR} ${OPENWRT_TOOLCHAIN_DIR})

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)


message(STATUS "OpenWRT staging....: '${OPENWRT_STAGING_DIR}'")
message(STATUS "OpenWRT toolchain..: '${OPENWRT_TOOLCHAIN_DIR}'")
message(STATUS "OpenWRT target.....: '${OPENWRT_TARGET_DIR}'")
message(STATUS "OpenWRT gcc .......: '${openwrt_c_compiler}'")
message(STATUS "OpenWRT g++ .......: '${openwrt_cxx_compiler}'")
