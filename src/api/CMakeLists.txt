# -*- mode: cmake; -*-


set(api_srcs
  Volkszaehler.cpp
  MySmartGrid.cpp
  CurlIF.cpp
  CurlCallback.cpp
  CurlResponse.cpp
)

add_library(vz-api ${api_srcs})