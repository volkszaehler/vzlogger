include_directories(
    ${GMOCK_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
)

# all *.cpp files here will be used.
file(GLOB test_sources *.cpp)

add_executable(vzlogger_unit_tests ${test_sources})

target_link_libraries(vzlogger_unit_tests
    ${GTEST_LIBS_DIR}/libgtest.a
    ${GMOCK_LIBS_DIR}/libgmock.a
    ${JSON_LIBRARY}
    ${SML_LIBRARY}
    ${LIBUUID}
    dl
    pthread)
target_link_libraries(vzlogger_unit_tests ${CURL_STATIC_LIBRARIES} ${CURL_LIBRARIES} ${GNUTLS_LIBRARIES})

