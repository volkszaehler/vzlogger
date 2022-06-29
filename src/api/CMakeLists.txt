# -*- mode: cmake; -*-


set(api_srcs
  Volkszaehler.cpp
  MySmartGrid.cpp
  InfluxDB.cpp
  Null.cpp
  CurlIF.cpp
  CurlCallback.cpp
  CurlResponse.cpp
  hmac.cpp
)

add_library(vz-api ${api_srcs})
install(TARGETS vz-api
	ARCHIVE DESTINATION lib/static COMPONENT libraries
	LIBRARY DESTINATION lib COMPONENT libraries)
