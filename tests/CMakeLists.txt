#include_directories(
#    ${GMOCK_INCLUDE_DIRS}
#    ${GTEST_INCLUDE_DIRS}
#)

# all *.cpp files here will be used.
file(GLOB test_sources *.cpp)
if(NOT SML_FOUND)
  list(REMOVE_ITEM test_sources ${CMAKE_CURRENT_SOURCE_DIR}/MeterSML.cpp)
endif(NOT SML_FOUND)

# remove meterOCR.cpp if not defined
if( NOT OCR_SUPPORT )
list(REMOVE_ITEM test_sources ${CMAKE_CURRENT_SOURCE_DIR}/ut_MeterOCR.cpp)
endif( NOT OCR_SUPPORT)

if( NOT OCR_TESSERACT_SUPPORT )
list(REMOVE_ITEM test_sources ${CMAKE_CURRENT_SOURCE_DIR}/ut_MeterOCRTesseract.cpp)
endif( NOT OCR_TESSERACT_SUPPORT)


if( NOT ENABLE_SML )
list(REMOVE_ITEM test_sources ${CMAKE_CURRENT_SOURCE_DIR}/MeterSML.cpp)
endif( NOT ENABLE_SML)

if(ENABLE_MQTT)
	set(MQTT_SOURCES ../src/mqtt.cpp)
else(ENABLE_MQTT)
	set(MQTT_SOURCES "")
endif(ENABLE_MQTT)

if( OMS_SUPPORT )
  set(oms_sources ../src/protocols/MeterOMS.cpp)
elseif( OMS_SUPPORT )
  set(oms_sources "")
endif( OMS_SUPPORT )

add_executable(vzlogger_unit_tests ${test_sources} ../src/CurlSessionProvider.cpp ../src/protocols/MeterW1therm.cpp ${oms_sources} ${MQTT_SOURCES})
target_link_libraries(vzlogger_unit_tests
    gtest
    gmock
    ${JSON_LIBRARY}
    ${LIBUUID}
    dl
    pthread)
target_link_libraries(vzlogger_unit_tests ${CURL_STATIC_LIBRARIES} ${CURL_LIBRARIES} unistring ${GNUTLS_LIBRARIES} ${OCR_LIBRARIES})

if( OMS_SUPPORT )
target_link_libraries(vzlogger_unit_tests ${MBUS_LIBRARY} ${OPENSSL_LIBRARIES})
endif( OMS_SUPPORT )

if(SML_FOUND)
  target_link_libraries(vzlogger_unit_tests ${SML_LIBRARY})
endif(SML_FOUND)

if(ENABLE_MQTT)
	target_link_libraries(vzlogger_unit_tests ${MQTT_LIBRARY})
endif(ENABLE_MQTT)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")

add_subdirectory(mocks)

FIND_PROGRAM( GCOV_PATH gcov )
FIND_PROGRAM( LCOV_PATH lcov )
FIND_PROGRAM( GENHTML_PATH genhtml )

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

ENDIF() # NOT GCOV_PATH
configure_file(../etc/vzlogger.conf etc/vzlogger.conf COPYONLY)

