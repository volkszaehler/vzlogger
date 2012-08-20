# -*- mode: cmake; -*-

# generic support code, provides the kde_ctest_setup() macro, which sets up everything required:
get_filename_component(_currentDir "${CMAKE_CURRENT_LIST_FILE}" PATH)
include( "${_currentDir}/KDECTestNightly.cmake")

macro(my_ctest_setup)
  set(_git_default_branch "c++-port")

  # init branch to checkout
  if( NOT _git_branch )
    set(_git_branch ${_git_default_branch})
  endif( NOT _git_branch )

  # initialisation target architectur
  if(NOT TARGET_ARCH)
    message("=1======> Setup Arch: ${TARGET_ARCH}")
    set(CTEST_BUILD_ARCH "linux")
  else(NOT TARGET_ARCH)
    message("=2======> Setup Arch: ${TARGET_ARCH}")
    set(CTEST_BUILD_ARCH ${TARGET_ARCH})
    if(${TARGET_ARCH} MATCHES "ar71xx")
      message("=3======> Setup Arch: ${TARGET_ARCH}")
      set(CMAKE_TOOLCHAIN_FILE /usr/local/OpenWrt-SDK-ar71xx-for-Linux-x86_64-gcc-4.3.3+cs_uClibc-0.9.30.1/staging_dir/host/Modules/Toolchain-OpenWRT.cmake)
    else(${TARGET_ARCH} MATCHES "ar71xx")
      set(CMAKE_TOOLCHAIN_FILE "CMAKE_TOOLCHAIN_FILE-NO_FOUND")
    endif(${TARGET_ARCH} MATCHES "ar71xx")
  endif(NOT TARGET_ARCH)

  # initialisation compiler
  if( NOT compiler )
    set(COMPILER_ID "gcc")
  else( NOT compiler )
    if( ${compiler} STREQUAL "clang" )
      find_program( CLANG_CC clang )
      find_program( CLANG_CXX clang++ )
      if( ${CLANG_CC} STREQUAL "CLANG_CC-NOTFOUND" OR ${CLANG_CXX} STREQUAL "CLANG_CC-NOTFOUND")
	message(FATAL_ERROR "clang compiler not found. stopping here.")
      else( ${CLANG_CC} STREQUAL "CLANG_CC-NOTFOUND" OR ${CLANG_CXX} STREQUAL "CLANG_CC-NOTFOUND")
	set(ENV{CC} ${CLANG_CC})
	set(ENV{CXX} ${CLANG_CXX})
	set(COMPILER_ID "clang")
      endif( ${CLANG_CC} STREQUAL "CLANG_CC-NOTFOUND" OR ${CLANG_CXX} STREQUAL "CLANG_CC-NOTFOUND")
    else( ${compiler} STREQUAL "clang" )
      message(FATAL_ERROR "Error. Compiler '${compiler}' not found. Stopping here.")
    endif( ${compiler} STREQUAL "clang" )
  endif( NOT compiler )

  # init branch to checkout
  if( NOT CTEST_PUSH_PACKAGES )
    set(CTEST_PUSH_PACKAGES 0)
  endif( NOT CTEST_PUSH_PACKAGES )

  # setup CTEST_BUILD_NAME
  if( ${_git_branch} STREQUAL ${_git_default_branch} )
    set(CTEST_BUILD_NAME "${CTEST_BUILD_ARCH}-${COMPILER_ID}-default")
  else( ${_git_branch} STREQUAL ${_git_default_branch} )
    set(CTEST_BUILD_NAME "${CTEST_BUILD_ARCH}-${COMPILER_ID}-default-${_git_branch}")
  endif( ${_git_branch} STREQUAL ${_git_default_branch} )

  set(_projectNameDir "${CTEST_PROJECT_NAME}")
  set(_srcDir "srcdir")
  set(_buildDir "builddir")

  set(GIT_UPDATE_OPTIONS "pull")

endmacro()

#
#
#
function(create_project_xml) 
  set(projectFile "${CTEST_BINARY_DIRECTORY}/Project.xml")
  file(WRITE ${projectFile}  "<Project name=\"${CTEST_PROJECT_NAME}\">")
  
  foreach(subproject ${CTEST_PROJECT_SUBPROJECTS})
    file(APPEND ${projectFile}
      "<SubProject name=\"${subproject}\">
</SubProject>
")
  endforeach()
  file(APPEND ${projectFile}
    "</Project>
")
  ctest_submit(FILES "${CTEST_BINARY_DIRECTORY}/Project.xml") 
endfunction()

#
#
#
macro(configure_ctest_config _CTEST_VCS_REPOSITORY configfile)
  string(REGEX REPLACE "[ /:\\.]" "_" _tmpDir ${_CTEST_VCS_REPOSITORY})
  set(_tmpDir "${_CTEST_DASHBOARD_DIR}/tmp/${_tmpDir}")
  configure_file(${configfile} ${_tmpDir}/CTestConfig.cmake COPYONLY)
  set(ctest_config ${_tmpDir}/CTestConfig.cmake)
endmacro()

#
#
#
function(FindOS OS_NAME OS_VERSION)

  set(_LinuxReleaseFiles
    redhat-release
    SuSE-release
    lsb-release
    )

  foreach(releasefile ${_LinuxReleaseFiles})
    if(EXISTS "/etc/${releasefile}" )
      file(READ "/etc/${releasefile}" _data )
      if( ${releasefile} MATCHES "redhat-release" )
	ReadRelease_redhat(${_data})# OS_NAME OS_VERSION)
      elseif( ${releasefile} MATCHES "SuSE-release" )
	ReadRelease_SuSE(${_data})# OS_NAME OS_VERSION)
      elseif( ${releasefile} MATCHES "lsb-release" )
	ReadRelease_lsb(${_data})# OS_NAME OS_VERSION)
      endif( ${releasefile} MATCHES "redhat-release" )
    endif(EXISTS "/etc/${releasefile}" )
  endforeach()
  #message("  OS_NAME   :  ${OS_NAME}")
  #message("  OS_VERSION:  ${OS_VERSION}")
  set(OS_NAME ${OS_NAME} PARENT_SCOPE)
  set(OS_VERSION ${OS_VERSION} PARENT_SCOPE)
endfunction(FindOS)

macro(ReadRelease_redhat _data OS_NAME OS_VERSION)
  # CentOS release 5.5 (Final)
  set(OS_NAME "CentOS")
  set(OS_NAME "Fedora")
  
endmacro()

macro(ReadRelease_SuSE _data OS_NAME OS_VERSION)
  # openSUSE 11.1 (x86_64)
  # VERSION = 11.1

  set(OS_NAME "openSUSE")
  string(REGEX REPLACE "VERSION=\"(.*)\"" "\\1" OS_VERSION ${_data})

endmacro()

macro(ReadRelease_lsb _data)
  # DISTRIB_ID=Ubuntu
  # DISTRIB_RELEASE=10.04
  # DISTRIB_CODENAME=lucid
  # DISTRIB_DESCRIPTION="Ubuntu 10.04.4 LTS"
  string(REGEX MATCH "DISTRIB_ID=[a-zA-Z0-9]+" _dummy ${_data})
  string(REGEX REPLACE "DISTRIB_ID=(.*)" "\\1" OS_NAME ${_dummy})
  string(REGEX MATCH "DISTRIB_RELEASE=[a-zA-Z0-9.]+" _dummy ${_data})
  string(REGEX REPLACE "DISTRIB_RELEASE=(.*)" "\\1" OS_VERSION ${_dummy})
  #message("    OS_NAME   :  ${OS_NAME}")
  #message("    OS_VERSION:  ${OS_VERSION}")
endmacro()

MARK_AS_ADVANCED(
  OS_NAME
  OS_VERSION
  )

function(ctest_push_files packageDir distFile)

  set(_export_host "192.168.9.63")
  execute_process(
    COMMAND scp -p ${distFile} ${_export_host}:packages
    WORKING_DIRECTORY ${packageDir}
    )
endfunction(ctest_push_files)

