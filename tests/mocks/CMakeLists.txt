include_directories(BEFORE .)

add_executable(mock_metermap mock_metermap.cpp ../../src/Meter.cpp ../../src/Options.cpp
	../../src/protocols/MeterD0.cpp
	../../src/protocols/MeterFile.cpp
	../../src/protocols/MeterExec.cpp
	../../src/protocols/MeterS0.cpp
	../../src/protocols/MeterRandom.cpp
	../../src/protocols/MeterSML.cpp
	../../src/protocols/MeterFluksoV2.cpp
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
)
target_link_libraries(mock_metermap ${CURL_STATIC_LIBRARIES} ${CURL_LIBRARIES} ${GNUTLS_LIBRARIES} ${OPENSSL_LIBRARIES})

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


add_test(mock_metermap mock_metermap)
