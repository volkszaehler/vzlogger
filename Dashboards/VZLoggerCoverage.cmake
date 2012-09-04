# -*- mode: cmake; -*-

# Todo
# * git update
# * cd openwrt
# * svn co openwrt....
# * modify feeds.conf
# * feeds update
# * feeds install

set(ENV{https_proxy} "http://squid.itwm.fhg.de:3128/")
include(Tools.cmake)
my_ctest_setup()
include(CTestConfigVZlogger.cmake)
# set(_ctest_type "Nightly")
# set(_ctest_type "Continuous")
set(_ctest_type "Coverage")

set(URL "https://github.com/kaikrueger/vzlogger.git")

set(CTEST_BASE_DIRECTORY "${KDE_CTEST_DASHBOARD_DIR}/${_projectNameDir}/${_ctest_type}")
set(CTEST_SOURCE_DIRECTORY "${CTEST_BASE_DIRECTORY}/${_srcDir}-${_git_branch}" )
set(CTEST_BINARY_DIRECTORY "${CTEST_BASE_DIRECTORY}/${_buildDir}-${CTEST_BUILD_NAME}")
set(CTEST_INSTALL_DIRECTORY "${CTEST_BASE_DIRECTORY}/install-${CTEST_BUILD_NAME}")
set(KDE_CTEST_VCS "git")
set(KDE_CTEST_VCS_REPOSITORY ${URL})

set(CMAKE_INSTALL_PREFIX "/usr")
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")

configure_ctest_config(${KDE_CTEST_VCS_REPOSITORY} "CTestConfigVZlogger.cmake")
kde_ctest_setup()

FindOS(OS_NAME OS_VERSION)

set(ctest_config ${CTEST_BASE_DIRECTORY}/CTestConfig.cmake)
#######################################################################
ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})

find_program(CTEST_GIT_COMMAND NAMES git)
set(CTEST_UPDATE_TYPE git)

set(CTEST_UPDATE_COMMAND  ${CTEST_GIT_COMMAND})
if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}/.git/HEAD")
  set(CTEST_CHECKOUT_COMMAND "${CTEST_GIT_COMMAND} clone ${URL} ${CTEST_SOURCE_DIRECTORY}")
endif(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}/.git/HEAD")

ctest_start(${_ctest_type})
ctest_update(SOURCE ${CTEST_SOURCE_DIRECTORY})
ctest_submit(PARTS Update)

execute_process(
  COMMAND ${CTEST_GIT_COMMAND} checkout  ${_git_branch}
  WORKING_DIRECTORY ${CTEST_SOURCE_DIRECTORY}
  )

# to get CTEST_PROJECT_SUBPROJECTS definition:

set(ENABLE_CODECOVERAGE 1)
set(CMAKE_BUILD_TYPE Profile)
set(CMAKE_ADDITIONAL_PATH ${CTEST_INSTALL_DIRECTORY})
#set(blah "${CTEST_INSTALL_DIRECTORY}/lib")

if(CMAKE_TOOLCHAIN_FILE)
  kde_ctest_write_initial_cache("${CTEST_BINARY_DIRECTORY}"
    CMAKE_TOOLCHAIN_FILE
    CMAKE_INSTALL_PREFIX
    CMAKE_ADDITIONAL_PATH
    ENABLE_CODECOVERAGE
    CMAKE_BUILD_TYPE
    )
else(CMAKE_TOOLCHAIN_FILE)
  kde_ctest_write_initial_cache("${CTEST_BINARY_DIRECTORY}"
    CMAKE_ADDITIONAL_PATH
    ENABLE_CODECOVERAGE
    CMAKE_BUILD_TYPE
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

# do codecoverage now
ctest_coverage(BUILD "${CTEST_BINARY_DIRECTORY}" RETURN_VALUE res)
message(STATUS "===> ctest_coverage: res='${res}'")

# do codecoverage now
ctest_memcheck(BUILD "${CTEST_BINARY_DIRECTORY}" RETURN_VALUE res)
message(STATUS "===> ctest_memcheck: res='${res}'")

ctest_test(BUILD "${CTEST_BINARY_DIRECTORY}" RETURN_VALUE test_res)
message("====> TESTS: ${test_res}")

ctest_submit(RETURN_VALUE res)

message("DONE")
