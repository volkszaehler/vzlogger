# -*- mode: cmake; -*-

if(VZ_BUILD_ON_PICO)
 set(api_srcs
     Volkszaehler.cpp
     Null.cpp
     LwipIF.cpp)
else(VZ_BUILD_ON_PICO)
 set(api_srcs
  Volkszaehler.cpp
  Null.cpp
  CurlIF.cpp
  CurlCallback.cpp
  CurlResponse.cpp
  hmac.cpp
)
endif(VZ_BUILD_ON_PICO)

if(VZ_USE_INFLUXDB)
  set(api_srcs_influxdb InfluxDB.cpp)
else(VZ_USE_INFLUXDB)
  set(api_srcs_influxdb "")
endif(VZ_USE_INFLUXDB)

if(VZ_USE_MYSMARTGRID)
  set(api_srcs_mysmartgrid MySmartGrid.cpp)
else(VZ_USEMYSMARTGRID)
  set(api_srcs_mysmartgrid "")
endif(VZ_USE_MYSMARTGRID)

if(VZ_BUILD_ON_PICO)
  add_library(vz-api STATIC ${api_srcs} ${api_srcs_influxdb} ${api_srcs_mysmartgrid})
else(VZ_BUILD_ON_PICO)
  add_library(vz-api ${api_srcs} ${api_srcs_influxdb} ${api_srcs_mysmartgrid})
endif(VZ_BUILD_ON_PICO)

install(TARGETS vz-api
	ARCHIVE DESTINATION lib/static COMPONENT libraries
	LIBRARY DESTINATION lib COMPONENT libraries)

if(VZ_BUILD_ON_PICO)
 target_compile_options(vz-api PUBLIC -fexceptions -DVZ_PICO)
endif(VZ_BUILD_ON_PICO)

