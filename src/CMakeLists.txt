# -*- mode: cmake; -*-

#AM_CFLAGS = -Wall -D_REENTRANT -std=gnu99 $(DEPS_VZ_CFLAGS)
#AM_CPPFLAGS = -I $(top_srcdir)/include
#AM_LDFLAGS =

option(VZ_BUILD_ON_PICO "Build on pico" ON)

option(VZ_USE_THREADS "Use threads" ON)
option(VZ_USE_INFLUXDB "Use influxdb" ON)
option(VZ_USE_MYSMARTGRID "Use mysmartgrid" ON)
option(VZ_USE_CURL "Use CURL as communcation lib" ON)

option(VZ_USE_METER_D0 "" ON)
option(VZ_USE_METER_EXEC "" ON)
option(VZ_USE_METER_FILE "" ON)
option(VZ_USE_METER_FLUKSOV2 "" ON)
option(VZ_USE_METER_OCR "" ON)
option(VZ_USE_METER_OCRTESSERACT "" ON)
option(VZ_USE_METER_OMS "" ON)
option(VZ_USE_METER_RANDOM "" ON)
option(VZ_USE_METER_S0 "" ON)
option(VZ_USE_METER_SML "" ON)
option(VZ_USE_METER_W1THERM "" ON)
option(VZ_USE_METER_PICO_SCT013 "" OFF)
option(VZ_USE_METER_PICO_ONBOARD_TEMP "" OFF)

option(VZ_USE_HTTPS "" OFF)

if(VZ_BUILD_ON_PICO)
  message(STATUS "Building VZ for Raspberry Pico ...")

  include_directories(${CMAKE_SOURCE_DIR}/include)
  include_directories(${CMAKE_SOURCE_DIR}/libs)
  include_directories(${CMAKE_SOURCE_DIR})
  include_directories(${CURL_SOURCE_DIR}/include)

  include_directories(${PICO_SDK_PATH}/lib/lwip/src/include/compat/posix
                    ${PICO_LWIP_PATH}/src/include
                    ${CMAKE_CURRENT_LIST_DIR}/..
                    ${CMAKE_CURRENT_LIST_DIR}/../..
                    ${PICO_SDK_PATH}/src/rp2_common/pico_lwip/include     # for arch/cc.h
                    ${PICO_SDK_PATH}/src/rp2_common/pico_stdio/include
                    ${PICO_SDK_PATH}/src/rp2_common/hardware_uart/include
                    ${PICO_SDK_PATH}/src/rp2_common/hardware_adc/include
                    ${PICO_SDK_PATH}/src/rp2_common/pico_cyw43_arch/include
                    ${PICO_SDK_PATH}/src/rp2_common/pico_cyw43_driver/include
                    ${PICO_SDK_PATH}/src/rp2_common/pico_async_context/include
                    ${PICO_SDK_PATH}/lib/mbedtls/include
                    ${PICO_SDK_PATH}/src/common/pico_stdlib/include
                    ${PICO_SDK_PATH}/lib/cyw43-driver/src
                    $<TARGET_PROPERTY:pico_lwip_core_headers,INTERFACE_INCLUDE_DIRECTORIES>
                    $<TARGET_PROPERTY:pico_base_headers,INTERFACE_INCLUDE_DIRECTORIES>
                    $<TARGET_PROPERTY:pico_rand,INTERFACE_INCLUDE_DIRECTORIES>
  )

# Build the absolute minimum - switch on as needed:
  set(VZ_USE_THREADS OFF)
  set(VZ_USE_INFLUXDB OFF)
  set(VZ_USE_MYSMARTGRID OFF)
  set(VZ_USE_METER_D0 OFF)
  set(VZ_USE_METER_EXEC OFF)
  set(VZ_USE_METER_FILE OFF)
  set(VZ_USE_METER_FLUKSOV2 OFF)
  set(VZ_USE_METER_OCR OFF)
  set(VZ_USE_METER_OCRTESSERACT OFF)
  set(VZ_USE_METER_OMS OFF)
  set(VZ_USE_METER_RANDOM OFF)
  set(VZ_USE_METER_S0 OFF)
  set(VZ_USE_METER_SML OFF)
  set(VZ_USE_METER_W1THERM OFF)

  set(VZ_USE_METER_PICO_SCT013 ON)
  set(VZ_USE_METER_PICO_ONBOARD_TEMP ON)

  set(VZ_USE_CURL OFF)
  set(VZ_USE_ALTCP ON)
  set(VZ_USE_HTTPS OFF)

  add_compile_options(-DPICO_DEBUG_MALLOC=1 -DPICO_PANIC_FUNCTION=vzlogger_panic)

endif(VZ_BUILD_ON_PICO)

add_subdirectory(api)
add_subdirectory(protocols)

## local interface support
#####################################################################
if(LOCAL_SUPPORT)
  set(local_srcs local.cpp)
  include_directories(${MICROHTTPD_INCLUDE_DIR})
else(LOCAL_SUPPORT)
  set(local_srcs "")
endif(LOCAL_SUPPORT)

if(ENABLE_MQTT)
  set(mqtt_srcs mqtt.cpp)
else(ENABLE_MQTT)
  set(mqtt_srcs "")
endif(ENABLE_MQTT)

configure_file("${CMAKE_SOURCE_DIR}/src/gitSha1.cpp.in" "${CMAKE_BINARY_DIR}/gitSha1.cpp" @ONLY)

if(VZ_BUILD_ON_PICO)
  set(vzlogger_srcs
    vzlogger_pico.cpp
    Meter.cpp
    Ntp.cpp
  )
else(VZ_BUILD_ON_PICO)
  set(vzlogger_srcs
    vzlogger.cpp
    ltqnorm.cpp
    Meter.cpp
    ${CMAKE_BINARY_DIR}/gitSha1.cpp
    CurlSessionProvider.cpp
    PushData.cpp ../include/PushData.hpp
  )
endif(VZ_BUILD_ON_PICO)

set(libvz_srcs
  Channel.cpp
  Config_Options.cpp
  Buffer.cpp
  Obis.cpp
  Options.cpp
  Reading.cpp
  exception.cpp
  ${local_srcs}
  ${mqtt_srcs}
  MeterMap.cpp
  )

if(VZ_USE_THREADS)
 set(libvz_srcs_threads threads.cpp)
else(VZ_USE_THREADS)
 set(libvz_srcs_threads "")
endif( VZ_USE_THREADS )

if(VZ_BUILD_ON_PICO)
  add_library(vz STATIC ${libvz_srcs} ${libvz_srcs_threads})
else(VZ_BUILD_ON_PICO)
  add_library(vz ${libvz_srcs} ${libvz_srcs_threads})
endif(VZ_BUILD_ON_PICO)

add_executable(vzlogger ${vzlogger_srcs})

if(VZ_BUILD_ON_PICO)

  if(VZ_USE_HTTPS)
   set(vz_pico_tls_libs pico_lwip_mbedtls pico_mbedtls)
   target_compile_options(vz-api PUBLIC -DVZ_USE_HTTPS)
  else(VZ_USE_HTTPS)
   set(vz_pico_tls_libs "")
  endif( VZ_USE_HTTPS )

  target_compile_options(vz PUBLIC -fexceptions -DVZ_PICO)
  target_link_libraries(vzlogger
          pico_cyw43_arch_lwip_threadsafe_background
          ${vz_pico_tls_libs}
          pico_stdlib
          hardware_adc
          proto vz vz-api
          json-c
          )
#  target_link_libraries(vzlogger -fno-omit-frame-pointer -mapcs-frame -funwind-tables -Wl,-wrap,malloc -Wl,-wrap,calloc -Wl,-wrap,realloc -Wl,-wrap,free)

  pico_enable_stdio_usb(vzlogger 1)
  pico_add_extra_outputs(vzlogger)
else(VZ_BUILD_ON_PICO)
  target_link_libraries(vzlogger ${LIBGCRYPT})
  target_link_libraries(vzlogger pthread m ${LIBUUID})
  target_link_libraries(vzlogger dl)
  target_link_libraries(vzlogger ${CURL_STATIC_LIBRARIES} ${CURL_LIBRARIES} unistring ${GNUTLS_LIBRARIES} ${OPENSSL_LIBRARIES} )
  target_link_libraries(vzlogger ${JSON_LIBRARY})
  target_link_libraries(vzlogger proto vz vz-api)
endif(VZ_BUILD_ON_PICO)

if(SML_FOUND)
  target_link_libraries(vzlogger ${SML_LIBRARY})
endif(SML_FOUND)

if( MBUS_FOUND )
    target_link_libraries(vzlogger ${MBUS_LIBRARY})
endif( MBUS_FOUND )

target_link_libraries(vzlogger ${OCR_LIBRARIES})

if(LOCAL_SUPPORT)
target_link_libraries(vzlogger ${MICROHTTPD_LIBRARY})
endif(LOCAL_SUPPORT)

if(ENABLE_MQTT)
target_link_libraries(vzlogger ${MQTT_LIBRARY})
endif(ENABLE_MQTT)

if( TARGET )
  if( ${TARGET} STREQUAL "ar71xx")
    set_target_properties(vzlogger PROPERTIES LINK_FLAGS "-static")
    target_link_libraries(vzlogger ${OPENWRT_TARGET_DIR}/usr/lib/libssl.a)
    target_link_libraries(vzlogger ${OPENWRT_TARGET_DIR}/usr/lib/libcrypto.a)
    target_link_libraries(vzlogger ${OPENWRT_TARGET_DIR}/usr/lib/libz.a)
    target_link_libraries(vzlogger dl)
  endif( ${TARGET} STREQUAL "ar71xx")
endif( TARGET )

# add programs to the install target
INSTALL(PROGRAMS
  ${CMAKE_CURRENT_BINARY_DIR}/vzlogger
  DESTINATION bin)
install(TARGETS vz
	ARCHIVE DESTINATION lib/static COMPONENT libraries
	LIBRARY DESTINATION lib COMPONENT libraries)
