include_directories(
    ${GMOCK_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
)

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

if( OMS_SUPPORT )
  set(oms_sources ../src/protocols/MeterOMS.cpp)
elseif( OMS_SUPPORT )
  set(oms_sources "")
endif( OMS_SUPPORT )

add_executable(vzlogger_unit_tests ${test_sources} ../src/CurlSessionProvider.cpp ../src/protocols/MeterW1therm.cpp ${oms_sources})

target_link_libraries(vzlogger_unit_tests
    ${GTEST_LIBS_DIR}/libgtest.a
    ${GMOCK_LIBS_DIR}/libgmock.a
    ${JSON_LIBRARY}
    ${LIBUUID}
    dl
    pthread)
target_link_libraries(vzlogger_unit_tests ${CURL_STATIC_LIBRARIES} ${CURL_LIBRARIES} ${GNUTLS_LIBRARIES} ${OCR_LIBRARIES})

if( OMS_SUPPORT )
target_link_libraries(vzlogger_unit_tests ${MBUS_LIBRARY} ${OPENSSL_LIBRARIES})
endif( OMS_SUPPORT )

if(SML_FOUND)
  target_link_libraries(vzlogger_unit_tests ${SML_LIBRARY})
endif(SML_FOUND)

add_subdirectory(mocks)
