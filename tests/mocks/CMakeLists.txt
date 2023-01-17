include_directories(BEFORE .)

if(LOCAL_SUPPORT)
    set(mock_local_srcs ../../src/local.cpp)
endif(LOCAL_SUPPORT)

if(ENABLE_MQTT)
	set(mock_mqtt_sources ../../src/mqtt.cpp)
else(ENABLE_MQTT)
	set(mock_mqtt_sources "")
endif(ENABLE_MQTT)

if(OMS_SUPPORT)
    set(mock_oms_sources ../../src/protocols/MeterOMS.cpp)
elseif( OMS_SUPPORT )
    set(mock_oms_sources "")
endif( OMS_SUPPORT )

if(SML_FOUND)
    set(mock_sml_sources ../../src/protocols/MeterSML.cpp)
elseif(SML_FOUND)
    set(mock_sml_sources "")
endif(SML_FOUND)

add_executable(mock_metermap
    mock_metermap.cpp
    ../../src/Meter.cpp
    ../../src/Options.cpp
	../../src/protocols/MeterD0.cpp
	../../src/protocols/MeterFile.cpp
	../../src/protocols/MeterExec.cpp
	../../src/protocols/MeterS0.cpp
	../../src/protocols/MeterRandom.cpp
	${mock_sml_sources}
	../../src/protocols/MeterFluksoV2.cpp
	../../src/protocols/MeterW1therm.cpp
	../../src/Reading.cpp
	../../src/Obis.cpp
	../../src/ltqnorm.cpp
	../../src/MeterMap.cpp
	../../src/threads.cpp
	../../src/api/hmac.cpp
	../../src/Config_Options.cpp
	../../src/Buffer.cpp
	../../src/api/Volkszaehler.cpp
	../../src/api/MySmartGrid.cpp
	../../src/api/InfluxDB.cpp
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
	${mock_mqtt_sources}
)

target_link_libraries(mock_metermap ${CURL_STATIC_LIBRARIES} ${CURL_LIBRARIES})

if (MICROHTTPD_FOUND)
    target_link_libraries(mock_metermap ${MICROHTTPD_LIBRARY})
endif(MICROHTTPD_FOUND)

target_link_libraries(mock_metermap unistring ${GNUTLS_LIBRARIES} ${OPENSSL_LIBRARIES})
target_link_libraries(mock_metermap
    gtest
    gmock
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
if(ENABLE_MQTT)
	target_link_libraries(mock_metermap ${MQTT_LIBRARY})
endif(ENABLE_MQTT)


add_executable(mock_MeterW1therm
    mock_MeterW1therm.cpp
    ../../src/protocols/MeterW1therm.cpp
    ../../src/Reading.cpp
    ../../src/Obis.cpp
    ../../src/Options.cpp
)

target_link_libraries(mock_MeterW1therm
    gtest
    gmock
    pthread
    ${JSON_LIBRARY}
    ${LIBUUID}
    dl
)

add_test(mock_metermap mock_metermap)
add_test(mock_MeterW1therm mock_MeterW1therm)

if(OMS_SUPPORT)
add_executable(mock_MeterOMS
	mock_MeterOMS.cpp
	../../src/protocols/MeterOMS.cpp
	../../src/Reading.cpp
	../../src/Obis.cpp
	../../src/Options.cpp
)

target_link_libraries(mock_MeterOMS
    gtest
    gmock
    pthread
    ${JSON_LIBRARY}
    ${LIBUUID}
    ${MBUS_LIBRARY}
    ${OPENSSL_LIBRARIES}
    dl
)
add_test(mock_MeterOMS mock_MeterOMS)

endif(OMS_SUPPORT)

add_executable(mock_MeterS0
	mock_MeterS0.cpp
	../../src/protocols/MeterS0.cpp
	../../src/Reading.cpp
	../../src/Obis.cpp
	../../src/Options.cpp
)

target_link_libraries(mock_MeterS0
    gtest
    gmock
    pthread
    ${JSON_LIBRARY}
    ${LIBUUID}
    dl
)
add_test(mock_MeterS0 mock_MeterS0)

# ensure that all test binaries are added to tests/CMakeLists.txt test_coverage target
# (or find a better way to add them from here to the test_coverage target
