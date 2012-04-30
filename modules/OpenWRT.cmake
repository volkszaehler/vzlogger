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
  NAMES openwrt.cmake
  HINTS ${OPENWRT_HOME}/staging_dir
  DOC "find cross-compiler configuration"
  )

if( ${_openwrt_configuration} STREQUAL "_openwrt_configuration-NOTFOUND" )
  message(FATAL_ERROR  "OpenWRT cross-compiling environement not found.")
endif( ${_openwrt_configuration} STREQUAL "_openwrt_configuration-NOTFOUND" )
include(${_openwrt_configuration})

## search compiler
#find_program()

message(STATUS "===> found gcc in: '${openwrt_compiler}'")
# set(OPENWRT_HOME /homes/krueger/Project/MySmartGrid/backfire_ubuntu/build/openwrt)
set(OPENWRT_STAGING_DIR ${OPENWRT_HOME}/staging_dir)
set(OPENWRT_STAGING_DIR_HOST ${OPENWRT_STAGING_DIR}/host)
set(OPENWRT_TOOLCHAIN_DIR ${OPENWRT_STAGING_DIR}/${openwrt_toolchain})
set(OPENWRT_TARGET_DIR    ${OPENWRT_STAGING_DIR}/${openwrt_target})


set(ENV{STAGING_DIR}     ${OPENWRT_STAGING_DIR})
set(ENV{PKG_CONFIG_PATH} ${OPENWRT_TARGET_DIR}/usr/lib/pkgconfig)
message("export STAGING_DIR=${OPENWRT_STAGING_DIR}")

#compiler
SET(CMAKE_C_COMPILER   ${OPENWRT_TOOLCHAIN_DIR}/bin/mips-openwrt-linux-gcc)
SET(CMAKE_CXX_COMPILER ${OPENWRT_TOOLCHAIN_DIR}/bin/mips-openwrt-linux-g++)

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
