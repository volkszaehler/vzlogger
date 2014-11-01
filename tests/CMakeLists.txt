include_directories(
    ${gtest_SOURCE_DIR}/include
)

# all *.cpp files here will be used. No main() required as we link with gtest_main
file(GLOB test_sources *.cpp)

add_executable(vzlogger_unit_tests ${test_sources})
target_link_libraries(vzlogger_unit_tests gtest_main)
