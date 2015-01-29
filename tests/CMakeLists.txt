include_directories(
    ${GMOCK_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
)

# all *.cpp files here will be used.
file(GLOB test_sources *.cpp)
if(NOT SML_FOUND)
  list(REMOVE_ITEM test_sources ${CMAKE_CURRENT_SOURCE_DIR}/MeterSML.cpp)
endif(NOT SML_FOUND)

add_executable(vzlogger_unit_tests ${test_sources})

target_link_libraries(vzlogger_unit_tests
    ${GTEST_LIBS_DIR}/libgtest.a
    ${GMOCK_LIBS_DIR}/libgmock.a
    ${JSON_LIBRARY}
    ${LIBUUID}
    dl
    pthread)
target_link_libraries(vzlogger_unit_tests ${CURL_STATIC_LIBRARIES} ${CURL_LIBRARIES} ${GNUTLS_LIBRARIES})

if(SML_FOUND)
  target_link_libraries(vzlogger_unit_tests ${SML_LIBRARY})
endif(SML_FOUND)

add_subdirectory(mocks)
