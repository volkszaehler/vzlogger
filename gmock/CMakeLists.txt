cmake_minimum_required(VERSION 2.8.0)
project(gmock_builder C CXX)
include(ExternalProject)

ExternalProject_Add(googlemock
    SVN_REPOSITORY http://googlemock.googlecode.com/svn/tags/release-1.7.0/
    CMAKE_ARGS
     PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
# Disable install step
    INSTALL_COMMAND ""
# Avoid svn update each time on make
    UPDATE_COMMAND ""
)

# Specify include dir
ExternalProject_Get_Property(googlemock source_dir)
set(GMOCK_INCLUDE_DIRS ${source_dir}/include PARENT_SCOPE)
set(GTEST_INCLUDE_DIRS ${source_dir}/gtest/include PARENT_SCOPE)

# Specify MainTest's link libraries
ExternalProject_Get_Property(googlemock binary_dir)
set(GTEST_LIBS_DIR ${binary_dir}/gtest PARENT_SCOPE)
set(GMOCK_LIBS_DIR ${binary_dir} PARENT_SCOPE)

