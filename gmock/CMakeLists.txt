cmake_minimum_required(VERSION 2.8.0)
project(gmock_builder C CXX)
include(ExternalProject)

ExternalProject_Add(googlemock
    GIT_REPOSITORY https://github.com/google/googletest.git
    CMAKE_ARGS
     PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
# Disable install step
    INSTALL_COMMAND ""
# Avoid svn update each time on make
    UPDATE_COMMAND ""
)

# Specify include dir
ExternalProject_Get_Property(googlemock source_dir)
set(GMOCK_INCLUDE_DIRS ${source_dir}/googlemock/include PARENT_SCOPE)
set(GTEST_INCLUDE_DIRS ${source_dir}/googletest/include PARENT_SCOPE)

# Specify MainTest's link libraries
ExternalProject_Get_Property(googlemock binary_dir)
set(GTEST_LIBS_DIR ${binary_dir}/googlemock/gtest PARENT_SCOPE)
set(GMOCK_LIBS_DIR ${binary_dir}/googlemock PARENT_SCOPE)

