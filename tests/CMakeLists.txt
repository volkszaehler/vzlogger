include_directories(
    ${GTEST_INCLUDE_DIRS}
)

# all *.cpp files here will be used. No main() required as we link with gtest_main
file(GLOB test_sources *.cpp)

add_executable(vzlogger_unit_tests ${test_sources})

target_link_libraries(vzlogger_unit_tests
    ${GTEST_LIBS_DIR}/libgtest.a
    ${GTEST_LIBS_DIR}/libgtest_main.a
    ${JSON_LIBRARY}
    ${SML_LIBRARY}
    ${LIBUUID}
    pthread)
