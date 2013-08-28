# -*- mode: cmake; -*-

include_directories(
  ${OPENSSL_INCLUDE_DIR}
)

set(api_srcs
  Volkszaehler.cpp
  MySmartGrid.cpp
  CurlIF.cpp
  CurlCallback.cpp
  CurlResponse.cpp
)

add_library(vz-api ${api_srcs})
target_link_libraries(vz-api
  ${OPENSSL_CRYPTO_LIBRARY}
)
