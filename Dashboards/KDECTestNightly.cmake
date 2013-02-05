# -*- mode: cmake; -*-
# This file provides macros which make running Nightly builds easier, 
# see KDE/kdelibs/KDELibsNightly.cmake or kdesupport/automoc/Automoc4Nightly.cmake
# as example.
# It can be used completely independent from KDE, the "KDE_" prefix is just to
# show that the macros and variables or not ctest-builtin ones, but coming
# from this script.
#
#
# Beside the variables and macros this script provides, it also does one more thing,
# it converts command line arguments into ctest (cmake) variables.
# CMake has the option -D to set variables, which ctest is currently missing.
# But there is a work around, invoke ctest like this:
# $ ctest -S myscript.cmake,VAR1=foo,VAR2=bar
#
# By including this (KDECTestNightly.cmake) script, the variables VAR1 will be
# set to "foo" and VAR2 to "bar" respectively. This works for any number of variables.
# Processing lists (which have semicolons) may have issues.
#
#
# KDECTestNightly.cmake uses the following variables, which you may want to set
# before calling any macros from this file:
#
# KDE_CTEST_BUILD_SUFFIX - this string will be appended to CMAKE_SYSTEM_NAME to
# when setting CTEST_BUILD_NAME.
#
# KDE_CTEST_VCS - set it to "svn" (for KDE) or "cvs".
# Send me a mail (or even better a patch) if you need support for git.
#
# KDE_CTEST_VCS_REPOSITORY - set this to the VCS repository of the project, e.g.
# "https://svn.kde.org/home/kde/trunk/KDE/kdelibs" for kdelibs.
#
# KDE_CTEST_VCS_PATH - the path inside the repository. This can be empty for svn,
# but you have to set it for cvs.
#
# KDE_CTEST_AVOID_SPACES - by default, the source directory for the Nightly build
# will contains spaces ("src dir/") and also the build
# directory. If this breaks the compilation of your software,
# either fix your software, or set KDE_CTEST_AVOID_SPACES
# to TRUE.
#
# KDE_CTEST_DASHBOARD_DIR - by default, all dashboards will be under $HOME/Dashboards/.
# By setting KDE_CTEST_DASHBOARD_DIR to another location this
# can be modified.
#
# KDE_CTEST_PARALLEL_LEVEL - number of jobs to run in parallel, e.g. for make -jX.
#
#
# The following macros are provided:
#
# KDE_CTEST_SETUP() - This macro does most of the work you would otherwise
# have to do manually.
# It sets up the following variables:
# CTEST_BUILD_NAME - unchanged if already set, otherwise set to CMAKE_SYSTEM_NAME, with
# KDE_CTEST_BUILD_SUFFIX appended (if set)
# CTEST_BINARY_DIRECTORY - unchanged if already set, otherwise set to $HOME/Dashboards/<projectname>/build dir-${CTEST_BUILD_NAME}/
# Set KDE_CTEST_AVOID_SPACES to TRUE to avoid the space in the path
# CTEST_CMAKE_GENERATOR - unchanged if already set, otherwise set to "Unix Makefiles" on UNIX, the
# Windows part still needs some work
# CTEST_SOURCE_DIRECTORY - unchanged if already set, otherwise set to $HOME/Dashboards/<projectname>/src dir/
# Set KDE_CTEST_AVOID_SPACES to TRUE to avoid the space in the path
# CTEST_SITE, CTEST_BUILD_COMMAND, CTEST_CHECKOUT_COMMAND, CTEST_UPDATE_COMMAND
# CTEST_USE_LAUNCHERS - this is set to TRUE if it has not been already set to TRUE or FALSE by the caller.
#
# KDE_CTEST_INSTALL(<dir>) - Installs the project in the given directory.
# <dir> should always be ${CTEST_BINARY_DIRECTORY}.
#
# KDE_CTEST_WRITE_INITIAL_CACHE(<dir> [var1 var2 ... varN] ) - Writes an initial
# CMakeCache.txt to the directory <dir> (which should always
# be ${CTEST_BINARY_DIRECTORY}). This CMakeCache.txt will contain
# the listed variables and their values. If any of the variables
# have not been set, they won't be written into this cache.
# This is the only way to get predefined values to variables
# during the CMake run, as e.g. CMAKE_INSTALL_PREFIX.
#
#
# Alex <neundorf AT kde.org>

# Copyright (c) 2009 Alexander Neundorf, <neundorf@kde.org>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

###########################################################
# generic code
###########################################################

cmake_minimum_required(VERSION 2.6)

if(POLICY CMP0011)
cmake_policy(SET CMP0011 NEW)
endif(POLICY CMP0011)


# transform ctest script arguments of the form script.ctest,var1=value1,var2=value2
# to variables with the respective names set to the respective values
string(REPLACE "," ";" scriptArgs "${CTEST_SCRIPT_ARG}")
foreach(currentVar ${scriptArgs})
if ("${currentVar}" MATCHES "^([^=]+)=(.+)$" )
set("${CMAKE_MATCH_1}" "${CMAKE_MATCH_2}")
endif ("${currentVar}" MATCHES "^([^=]+)=(.+)$" )
endforeach(currentVar ${scriptArgs})


function(KDE_CTEST_WRITE_INITIAL_CACHE _dir)
if(NOT EXISTS "${_dir}")
file(MAKE_DIRECTORY "${_dir}")
endif(NOT EXISTS "${_dir}")
file(WRITE "${_dir}/CMakeCache.txt" "# CMakeCache.txt created by KDE_CTEST_WRITE_INITIAL_CACHE() macro\n" )

# By default, also check for the CTEST_USE_LAUNCHERS variable:
foreach(currentArg ${ARGN} CTEST_USE_LAUNCHERS)
if(DEFINED ${currentArg})
file(APPEND "${_dir}/CMakeCache.txt" "${currentArg}:STRING=${${currentArg}}\n\n")
endif(DEFINED ${currentArg})
endforeach(currentArg ${ARGN} CTEST_USE_LAUNCHERS)
endfunction(KDE_CTEST_WRITE_INITIAL_CACHE)


function(KDE_CTEST_INSTALL _dir)
if(EXISTS ${_dir})
if (EXISTS ${_dir}/cmake_install.cmake)
execute_process(COMMAND ${CMAKE_COMMAND} -P ${_dir}/cmake_install.cmake)
else (EXISTS ${_dir}/cmake_install.cmake)
message("Nothing found to install in ${_dir}")
endif (EXISTS ${_dir}/cmake_install.cmake)
else(EXISTS ${_dir})
message("KDE_CTEST_INSTALL(): directory ${_dir} does not exist")
endif(EXISTS ${_dir})
endfunction(KDE_CTEST_INSTALL _dir)


macro(KDE_CTEST_SETUP)

include(CMakeDetermineSystem)
if(CMAKE_HOST_UNIX)
include(Platform/UnixPaths)
endif(CMAKE_HOST_UNIX)

if(CMAKE_HOST_WIN32)
include(Platform/WindowsPaths)
endif(CMAKE_HOST_WIN32)

# Figure out which generator to use:
if(NOT DEFINED CTEST_CMAKE_GENERATOR)
  if(CMAKE_HOST_UNIX)
    # on any UNIX, use the plain "Unix Makefiles" generator for now:
    set(CTEST_CMAKE_GENERATOR "Unix Makefiles" )
  endif(CMAKE_HOST_UNIX)

  if(CMAKE_HOST_WIN32)
    # Under Windows, if cl.exe is in the path, generate nmake makefiles,
    # otherwise mingw.
    find_program(_KDECTEST_CL_EXE NAMES cl)
    if(_KDECTEST_CL_EXE)
      set(CTEST_CMAKE_GENERATOR "NMake Makefiles" )
    else(_KDECTEST_CL_EXE)
      set(CTEST_CMAKE_GENERATOR "MinGW Makefiles" )
    endif(_KDECTEST_CL_EXE)
  endif(CMAKE_HOST_WIN32)

endif(NOT DEFINED CTEST_CMAKE_GENERATOR)

site_name(CTEST_SITE)

if(KDE_CTEST_BUILD_SUFFIX)
set(KDE_CTEST_BUILD_SUFFIX "-${KDE_CTEST_BUILD_SUFFIX}")
endif(KDE_CTEST_BUILD_SUFFIX)

if(NOT CTEST_BUILD_NAME)
set(CTEST_BUILD_NAME ${CMAKE_SYSTEM_NAME}-CMake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}${KDE_CTEST_BUILD_SUFFIX})
endif(NOT CTEST_BUILD_NAME)


if("${CTEST_CMAKE_GENERATOR}" MATCHES Makefile)
  if("${CTEST_CMAKE_GENERATOR}" MATCHES NMake)
    find_program(MAKE_EXECUTABLE NAMES nmake)
  else("${CTEST_CMAKE_GENERATOR}" MATCHES NMake)
    find_program(MAKE_EXECUTABLE NAMES gmake make)
  endif("${CTEST_CMAKE_GENERATOR}" MATCHES NMake)

  set(CTEST_BUILD_COMMAND "${MAKE_EXECUTABLE}" )

if("${KDE_CTEST_PARALLEL_LEVEL}" GREATER 1)
set(CTEST_BUILD_FLAGS -j${KDE_CTEST_PARALLEL_LEVEL})
endif("${KDE_CTEST_PARALLEL_LEVEL}" GREATER 1)

else("${CTEST_CMAKE_GENERATOR}" MATCHES Makefile)
if(NOT DEFINED CTEST_BUILD_COMMAND)
message(FATAL_ERROR "CTEST_CMAKE_GENERATOR is set to \"${CTEST_CMAKE_GENERATOR}\", but CTEST_BUILD_COMMAND has not been set")
endif(NOT DEFINED CTEST_BUILD_COMMAND)
endif("${CTEST_CMAKE_GENERATOR}" MATCHES Makefile)

# By default set CTEST_USE_LAUNCHERS to TRUE if it has not been set by the caller.
# This gives much better error/warnings parsing.
if(NOT DEFINED CTEST_USE_LAUNCHERS)
set(CTEST_USE_LAUNCHERS TRUE)
endif(NOT DEFINED CTEST_USE_LAUNCHERS)

# VCS support step 1: figure out which system to use, find the tool,
# and get CTestConfig.cmake from the VCS, so we know the project name and
# the nightly start time:
string(TOLOWER ${KDE_CTEST_VCS} _ctest_vcs)
set(_have_vcs FALSE)
if ("${_ctest_vcs}" STREQUAL svn)
find_program(SVN_EXECUTABLE NAMES svn)
if (NOT SVN_EXECUTABLE)
message(FATAL_ERROR "Error: KDE_CTEST_VCS is svn, but could not find svn executable")
endif (NOT SVN_EXECUTABLE)
set(_have_vcs TRUE)
endif ("${_ctest_vcs}" STREQUAL svn)

if ("${_ctest_vcs}" STREQUAL cvs)
find_program(CVS_EXECUTABLE NAMES cvs cvsnt)
if (NOT CVS_EXECUTABLE)
message(FATAL_ERROR "Error: KDE_CTEST_VCS is cvs, but could not find cvs or cvsnt executable")
endif (NOT CVS_EXECUTABLE)
set(_have_vcs TRUE)
endif ("${_ctest_vcs}" STREQUAL cvs)

if ("${_ctest_vcs}" STREQUAL git)
find_program(CTEST_GIT_COMMAND git)
if (NOT CTEST_GIT_COMMAND)
message(FATAL_ERROR "Error: KDE_CTEST_VCS is git, but could not find git executable")
endif (NOT CTEST_GIT_COMMAND)
set(_have_vcs TRUE)
endif ("${_ctest_vcs}" STREQUAL git)

if(NOT _have_vcs)
message(FATAL_ERROR "Error: KDE_CTEST_VCS is \"${KDE_CTEST_VCS}\", which is currently not supported")
endif(NOT _have_vcs)


############# set up CTEST_SOURCE_DIRECTORY and CTEST_BINARY_DIRECTORY #############
if(NOT DEFINED KDE_CTEST_DASHBOARD_DIR)
if(WIN32)
set(KDE_CTEST_DASHBOARD_DIR "$ENV{USERPROFILE}/Dashboards" )
else(WIN32)
set(KDE_CTEST_DASHBOARD_DIR "$ENV{HOME}/Dashboards" )
endif(WIN32)
endif(NOT DEFINED KDE_CTEST_DASHBOARD_DIR)

#### Now that we have the VCS, get the CTestConfig.cmake file from it
string(REGEX REPLACE "[ /:\\.]" "_" _tmpDir ${KDE_CTEST_VCS_REPOSITORY})
set(_tmpDir "${KDE_CTEST_DASHBOARD_DIR}/tmp/${_tmpDir}")
message("_tmpDir is ${_tmpDir}")
file(MAKE_DIRECTORY "${_tmpDir}")

if ("${_ctest_vcs}" STREQUAL svn)
execute_process(COMMAND ${SVN_EXECUTABLE} export ${KDE_CTEST_VCS_REPOSITORY}/CTestConfig.cmake
WORKING_DIRECTORY "${_tmpDir}")
endif ("${_ctest_vcs}" STREQUAL svn)

if ("${_ctest_vcs}" STREQUAL cvs)
execute_process(COMMAND "${CVS_EXECUTABLE} -d ${KDE_CTEST_VCS_REPOSITORY} co -d \"${_tmpDir}\" ${KDE_CTEST_VCS_PATH}/CTestConfig.cmake"
WORKING_DIRECTORY "${_tmpDir}")
endif ("${_ctest_vcs}" STREQUAL cvs)


if ("${_ctest_vcs}" STREQUAL git)
# Two example git clone and corresponding gitweb addresses:
#http://anongit.kde.org/automoc
#http://gitweb.kde.org/automoc.git/blob_plain/HEAD:/CTestConfig.cmake
#http://gitweb.kde.org/gitweb?p=automoc.git;a=blob_plain;f=CTestConfig.cmake;hb=HEAD

#http://cmake.org/cmake.git
#http://cmake.org/gitweb?p=cmake.git;a=blob_plain;f=CTestConfig.cmake;hb=HEAD

if("${KDE_CTEST_VCS_REPOSITORY}" MATCHES ".+/([^/]+)(\\.git)?$")
set(_gitProject ${CMAKE_MATCH_1})
else()
message(FATAL_ERROR "Could not extract git repository name from ${KDE_CTEST_VCS_REPOSITORY}")
endif()

set(fullGitwebUrl "${KDE_CTEST_GITWEB_URL}/gitweb?p=${_gitProject}.git;a=blob_plain;f=CTestConfig.cmake;hb=HEAD")
file(DOWNLOAD "${fullGitwebUrl}" ${_tmpDir}/CTestConfig.cmake TIMEOUT 60 STATUS _downloadResult)
message("Downloading -${fullGitwebUrl}-")
# A bit ugly, but git doesn't (AFAIK) support checking out single files
# execute_process(COMMAND ${CTEST_GIT_COMMAND} clone ${KDE_CTEST_VCS_REPOSITORY} "${_tmpDir}"
# WORKING_DIRECTORY "${_tmpDir}")
endif ("${_ctest_vcs}" STREQUAL git)

include("${_tmpDir}/CTestConfig.cmake")
file(REMOVE_RECURSE "${_tmpDir}")

# Now that we included CTestConfig.cmake, we have the project name and nightly start time

if(KDE_CTEST_AVOID_SPACES)
string(REGEX REPLACE " " "" _projectNameDir "${CTEST_PROJECT_NAME}")
set(_srcDir "srcdir")
set(_buildDir "builddir")
else(KDE_CTEST_AVOID_SPACES)
set(_projectNameDir "${CTEST_PROJECT_NAME}")
set(_srcDir "src dir")
set(_buildDir "build dir")
endif(KDE_CTEST_AVOID_SPACES)

if(NOT DEFINED CTEST_SOURCE_DIRECTORY)
set(CTEST_SOURCE_DIRECTORY "${KDE_CTEST_DASHBOARD_DIR}/${_projectNameDir}/${_srcDir}" )
endif(NOT DEFINED CTEST_SOURCE_DIRECTORY)

if(NOT DEFINED CTEST_BINARY_DIRECTORY)
set(CTEST_BINARY_DIRECTORY "${KDE_CTEST_DASHBOARD_DIR}/${_projectNameDir}/${_buildDir}-${CTEST_BUILD_NAME}" )
endif(NOT DEFINED CTEST_BINARY_DIRECTORY)

############### set up VCS support ###################

# VCS support step 2: set up the update and checkout commands
# only set this if there is no checkout yet
set(CTEST_CHECKOUT_COMMAND)

if ("${_ctest_vcs}" STREQUAL svn)
set(CTEST_UPDATE_COMMAND ${SVN_EXECUTABLE})
if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}/.svn/entries")
# concatenate VCS_REPOSITORY and VCS_PATH, and remove trailing slashes:
string(REGEX REPLACE "/+$" "" _KDE_CTEST_SVN_URL "${KDE_CTEST_VCS_REPOSITORY}/${KDE_CTEST_VCS_PATH}")
set(CTEST_CHECKOUT_COMMAND "${SVN_EXECUTABLE} co ${_KDE_CTEST_SVN_URL} \"${CTEST_SOURCE_DIRECTORY}\"")
# set(CTEST_CHECKOUT_COMMAND "${SVN_EXECUTABLE} co ${KDE_CTEST_VCS_REPOSITORY}/${KDE_CTEST_VCS_PATH} \"${CTEST_SOURCE_DIRECTORY}\"")
endif(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}/.svn/entries")
endif ("${_ctest_vcs}" STREQUAL svn)

if ("${_ctest_vcs}" STREQUAL cvs)
set(CTEST_UPDATE_COMMAND ${CVS_EXECUTABLE})
if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}/CVS/Entries")
get_filename_component(_srcSubdir "${CTEST_SOURCE_DIRECTORY}" NAME)
set(CTEST_CHECKOUT_COMMAND "${CVS_EXECUTABLE} -d ${KDE_CTEST_VCS_REPOSITORY} co -d \"${_srcSubdir}\" ${KDE_CTEST_VCS_PATH}")
endif(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}/CVS/Entries")
endif ("${_ctest_vcs}" STREQUAL cvs)

if ("${_ctest_vcs}" STREQUAL git)
set(CTEST_UPDATE_COMMAND ${GIT_EXECUTABLE})
if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}/.git/HEAD")
set(CTEST_CHECKOUT_COMMAND "${CTEST_GIT_COMMAND} clone ${KDE_CTEST_VCS_REPOSITORY} \"${CTEST_SOURCE_DIRECTORY}\"")
SET(UPDATE_COMMAND "${GITCOMMAND}")
SET(UPDATE_OPTIONS "${GIT_UPDATE_OPTIONS}")
endif(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}/.git/HEAD")
endif ("${_ctest_vcs}" STREQUAL git)

endmacro(KDE_CTEST_SETUP)
