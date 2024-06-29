
# all *.cpp files here will be used.
file(GLOB test_sources *.cpp)

# add required source files
list(APPEND test_sources
    ../src/Buffer.cpp
    ../src/Channel.cpp
    ../src/Config_Options.cpp
    ../src/api/Volkszaehler.cpp
    ../src/CurlSessionProvider.cpp
    ../src/protocols/MeterW1therm.cpp
    ../src/api/hmac.cpp
)

set(test_libraries
    gtest
    gmock
    ${JSON_LIBRARY}
    ${LIBUUID}
    dl
    pthread
    ${CURL_STATIC_LIBRARIES}
    ${CURL_LIBRARIES}
    unistring
    ${GNUTLS_LIBRARIES}
    ${OCR_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)

if(SML_FOUND AND ENABLE_SML)
    list(APPEND test_sources ../src/protocols/MeterSML.cpp)
    list(APPEND test_libraries ${SML_LIBRARY})
else(SML_FOUND AND ENABLE_SML)
    list(REMOVE_ITEM test_sources ${CMAKE_CURRENT_SOURCE_DIR}/MeterSML.cpp)
endif(SML_FOUND AND ENABLE_SML)

if(OCR_SUPPORT)
    list(APPEND test_sources ../src/protocols/MeterOCR.cpp)
else(OCR_SUPPORT)
    list(REMOVE_ITEM test_sources ${CMAKE_CURRENT_SOURCE_DIR}/ut_MeterOCR.cpp)
endif(OCR_SUPPORT)

if(OCR_TESSERACT_SUPPORT)
    list(APPEND test_sources ../src/protocols/MeterOCR.cpp)
else(OCR_TESSERACT_SUPPORT)
    list(REMOVE_ITEM test_sources
        ${CMAKE_CURRENT_SOURCE_DIR}/ut_MeterOCRTesseract.cpp)
endif(OCR_TESSERACT_SUPPORT)

if(ENABLE_MQTT)
    list(APPEND test_sources ../src/mqtt.cpp)
    list(APPEND test_libraries ${MQTT_LIBRARY})
endif(ENABLE_MQTT)

if(OMS_SUPPORT)
    list(APPEND test_sources ../src/protocols/MeterOMS.cpp)
    list(APPEND test_libraries ${MBUS_LIBRARY})
endif(OMS_SUPPORT)

add_executable(vzlogger_unit_tests ${test_sources})
target_link_libraries(vzlogger_unit_tests ${test_libraries})

configure_file(include/test_config.hpp.in include/test_config.hpp)
target_include_directories(vzlogger_unit_tests
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include
)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")

add_subdirectory(mocks)

FIND_PROGRAM(GCOV_PATH gcov)
FIND_PROGRAM(LCOV_PATH lcov)
FIND_PROGRAM(GENHTML_PATH genhtml)

IF(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
    MESSAGE("gcov found. Adding target test_coverage ...")

    # Setup target for coverage
    # we need to add each single test binary into the command list

    ADD_CUSTOM_TARGET(test_coverage
        # Cleanup lcov
        ${LCOV_PATH} --rc lcov_branch_coverage=1 --directory . --zerocounters
        # Run tests
        COMMAND vzlogger_unit_tests
        COMMAND mock_metermap
        COMMAND mock_MeterW1therm
        COMMAND mock_MeterOMS
        COMMAND mock_MeterS0

        # Capturing lcov counters and generating report
        COMMAND ${LCOV_PATH} --rc lcov_branch_coverage=1 --directory . --capture --output-file coverage.info
        COMMAND ${LCOV_PATH} --rc lcov_branch_coverage=1 --remove coverage.info 'tests/*' '/usr/*' --output-file coverage.info.cleaned
        COMMAND ${GENHTML_PATH} --rc lcov_branch_coverage=1 -o coverage coverage.info.cleaned
        COMMAND ${CMAKE_COMMAND} -E remove coverage.info coverage.info.cleaned

        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Resetting code coverage counters to zero.\nProcessing code coverage counters and generating report."
    )

    # Show info where to find the report
    ADD_CUSTOM_COMMAND(TARGET test_coverage POST_BUILD
        COMMAND ;
        COMMENT "Open ./coverage/index.html in your browser to view the coverage report."
    )

ENDIF(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
