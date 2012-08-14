# -*- mode: cmake; -*-

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

function(configure_ctest_config _CTEST_VCS_REPOSITORY configfile)
  string(REGEX REPLACE "[ /:\\.]" "_" _tmpDir ${_CTEST_VCS_REPOSITORY})
  set(_tmpDir "${_CTEST_DASHBOARD_DIR}/tmp/${_tmpDir}")
  configure_file(${configfile} ${_tmpDir}/CTestConfig.cmake COPYONLY)
endfunction()

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
