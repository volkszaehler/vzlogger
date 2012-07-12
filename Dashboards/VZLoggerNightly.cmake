# -*- mode: cmake; -*-

# Todo
# * git update
# * cd openwrt
# * svn co openwrt....
# * modify feeds.conf
# * feeds update
# * feeds install

set($ENV{https_proxy} "http://squid.itwm.fhg.de:3128/")
include(CTestConfigVZlogger.cmake)

set(KDE_CTEST_DASHBOARD_DIR "/tmp/msgrid")
set(CTEST_BASE_DIRECTORY "${KDE_CTEST_DASHBOARD_DIR}/${_projectNameDir}")
set(CTEST_SOURCE_DIRECTORY "${CTEST_BASE_DIRECTORY}/${_srcDir}" )
set(CTEST_BINARY_DIRECTORY "${CTEST_BASE_DIRECTORY}/${_buildDir}-${CTEST_BUILD_NAME}")
set(KDE_CTEST_VCS "git")

set(CMAKE_INSTALL_PREFIX "/usr")
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")

# generic support code, provides the kde_ctest_setup() macro, which sets up everything required:
get_filename_component(_currentDir "${CMAKE_CURRENT_LIST_FILE}" PATH)
include( "${_currentDir}/KDECTestNightly.cmake")
kde_ctest_setup()

set(ctest_config ${CTEST_BASE_DIRECTORY}/CTestConfig.cmake)
#######################################################################
ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})


set(URL "https://github.com/kaikrueger/vzlogger.git")
find_program(CTEST_GIT_COMMAND NAMES git)
#set(CTEST_UPDATE_TYPE git)

set(CTEST_UPDATE_COMMAND  ${CTEST_GIT_COMMAND})
if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}/.git/HEAD")
  set(CTEST_CHECKOUT_COMMAND "${CTEST_GIT_COMMAND} clone ${URL} ${CTEST_SOURCE_DIRECTORY}")
endif(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}/.git/HEAD")

configure_file("CTestConfigVZlogger.cmake" ${CTEST_BASE_DIRECTORY}/CTestConfig.cmake COPYONLY)
ctest_empty_binary_directory("${CTEST_BINARY_DIRECTORY}")
ctest_start("Nightly")
ctest_update(SOURCE ${CTEST_SOURCE_DIRECTORY})

execute_process(
  COMMAND echo ${CTEST_GIT_COMMAND} checkout  ${_git_branch}
  COMMAND ${CTEST_GIT_COMMAND} checkout  ${_git_branch}
  WORKING_DIRECTORY ${CTEST_SOURCE_DIRECTORY}
  )

# to get CTEST_PROJECT_SUBPROJECTS definition:
include(${ctest_config})

if(CMAKE_TOOLCHAIN_FILE)
  kde_ctest_write_initial_cache("${CTEST_BINARY_DIRECTORY}"
    CMAKE_TOOLCHAIN_FILE
    CMAKE_INSTALL_PREFIX
    )
endif(CMAKE_TOOLCHAIN_FILE)


ctest_configure(BUILD "${CTEST_BINARY_DIRECTORY}"  RETURN_VALUE resultConfigure)
message("====> Configure: ${resultConfigure}")

if( STAGING_DIR)
  include(${CTEST_BINARY_DIRECTORY}/CMakeCache.txt)
  set(ENV{STAGING_DIR}     ${OPENWRT_STAGING_DIR})
endif( STAGING_DIR)

ctest_build(BUILD "${CTEST_BINARY_DIRECTORY}" RETURN_VALUE build_res)
message("====> BUILD: ${build_res}")

ctest_submit(RETURN_VALUE res)

# package files
include(${CTEST_BINARY_DIRECTORY}/CPackConfig.cmake)
if( STAGING_DIR)
  set(ENV{PATH}            ${OPENWRT_STAGING_DIR}/host/bin:$ENV{PATH})
endif( STAGING_DIR)

if( NOT ${build_res})
  execute_process(
    COMMAND cpack -G DEB
#    COMMAND cpack -G TGZ
    WORKING_DIRECTORY ${CTEST_BINARY_DIRECTORY}
    )
endif( NOT ${build_res})

# upload files
if( NOT ${build_res})
  if(CPACK_ARCHITECTUR)
    set(OPKG_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_ARCHITECTUR}")
    set(_package_file "${OPKG_FILE_NAME}.ipk")
  else(CPACK_ARCHITECTUR)
    set(_package_file "${CPACK_PACKAGE_FILE_NAME}.deb")
  endif(CPACK_ARCHITECTUR)
  message("==> Upload packages - ${_package_file}")
  set(_export_host "192.168.9.63")
  execute_process(
    COMMAND ssh ${_export_host} mkdir -p packages
    COMMAND scp -p ${_package_file} ${_export_host}:packages/
    WORKING_DIRECTORY ${CTEST_BINARY_DIRECTORY}
    )
endif( NOT ${build_res})

message("DONE")
