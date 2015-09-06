include_directories(BEFORE .)

if(LOCAL_SUPPORT)
  set(mock_local_srcs ../../src/local.cpp)
endif(LOCAL_SUPPORT)

if( OMS_SUPPORT )
  set(mock_oms_sources ../../src/protocols/MeterOMS.cpp)
elseif( OMS_SUPPORT )
  set(mock_oms_sources "")
endif( OMS_SUPPORT )


add_executable(mock_metermap mock_metermap.cpp ../../src/Meter.cpp ../../src/Options.cpp
	../../src/protocols/MeterD0.cpp
	../../src/protocols/MeterFile.cpp
	../../src/protocols/MeterExec.cpp
	../../src/protocols/MeterS0.cpp
	../../src/protocols/MeterRandom.cpp
	../../src/protocols/MeterSML.cpp
	../../src/protocols/MeterFluksoV2.cpp
	../../src/protocols/MeterW1therm.cpp
	../../src/Reading.cpp
	../../src/Obis.cpp
	../../src/ltqnorm.cpp
	../../src/MeterMap.cpp
	../../src/threads.cpp
	../../src/Config_Options.cpp
	../../src/Buffer.cpp
	../../src/api/Volkszaehler.cpp
	../../src/api/MySmartGrid.cpp
	../../src/api/Null.cpp
	../../src/api/CurlIF.cpp
	../../src/api/CurlCallback.cpp
	../../src/api/CurlResponse.cpp
	protocols/MeterOCR.hpp
	Channel.hpp
	../../src/CurlSessionProvider.cpp
	../../src/PushData.cpp
	${mock_local_srcs}
	${mock_oms_sources}
)

target_link_libraries(mock_metermap ${CURL_STATIC_LIBRARIES} ${CURL_LIBRARIES} ${MICROHTTPD_LIBRARY} ${GNUTLS_LIBRARIES} ${OPENSSL_LIBRARIES})

target_link_libraries(mock_metermap 
		${GTEST_LIBS_DIR}/libgtest.a
		${GMOCK_LIBS_DIR}/libgmock.a
		pthread
		${JSON_LIBRARY}
		${LIBUUID}
		dl
	  )
if(SML_FOUND)
  target_link_libraries(mock_metermap ${SML_LIBRARY})
endif(SML_FOUND)
if(MBUS_FOUND)
    target_link_libraries(mock_metermap ${MBUS_LIBRARY})
endif(MBUS_FOUND)

add_executable(mock_MeterW1therm
    mock_MeterW1therm.cpp
    ../../src/protocols/MeterW1therm.cpp
    ../../src/Reading.cpp
    ../../src/Obis.cpp
    ../../src/Options.cpp
)

target_link_libraries(mock_MeterW1therm
		${GTEST_LIBS_DIR}/libgtest.a
		${GMOCK_LIBS_DIR}/libgmock.a
		pthread
		${JSON_LIBRARY}
		${LIBUUID}
		dl
)

add_test(mock_metermap mock_metermap)
add_test(mock_MeterW1therm mock_MeterW1therm)

if( OMS_SUPPORT )
add_executable(mock_MeterOMS
	mock_MeterOMS.cpp
	../../src/protocols/MeterOMS.cpp
	../../src/Reading.cpp
	../../src/Obis.cpp
	../../src/Options.cpp
)
target_link_libraries(mock_MeterOMS
		${GTEST_LIBS_DIR}/libgtest.a
		${GMOCK_LIBS_DIR}/libgmock.a
		pthread
		${JSON_LIBRARY}
		${LIBUUID}
		${MBUS_LIBRARY}
		${OPENSSL_LIBRARIES}
		dl
)
add_test(mock_MeterOMS mock_MeterOMS)

endif( OMS_SUPPORT )

add_executable(mock_MeterS0
	mock_MeterS0.cpp
	../../src/protocols/MeterS0.cpp
	../../src/Reading.cpp
	../../src/Obis.cpp
	../../src/Options.cpp
)

target_link_libraries(mock_MeterS0
		${GTEST_LIBS_DIR}/libgtest.a
		${GMOCK_LIBS_DIR}/libgmock.a
		pthread
		${JSON_LIBRARY}
		${LIBUUID}
		dl
)
add_test(mock_MeterS0 mock_MeterS0)

# ensure that all test binaries are added to tests/CMakeLists.txt test_coverage target
# (or find a better way to add them from here to the test_coverage target
