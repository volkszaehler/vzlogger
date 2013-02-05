# -*- mode: cmake; -*-

#AM_CFLAGS = -Wall -D_REENTRANT -std=gnu99 $(DEPS_VZ_CFLAGS)
#AM_CPPFLAGS = -I $(top_srcdir)/include
#AM_LDFLAGS =

add_subdirectory(api)
add_subdirectory(protocols)

## local interface support
#####################################################################
if(ENABLE_LOCAL)
  set(local_srcs local.cpp)
  include_directories(${MICROHTTPD_INCLUDE_DIR})
else(ENABLE_LOCAL)
  set(local_srcs "")
endif(ENABLE_LOCAL)


set(vzlogger_srcs
  vzlogger.cpp 
  ltqnorm.cpp
  Meter.cpp
)

set(libvz_srcs 
  Channel.cpp
  Config_Options.cpp
  threads.cpp
  Buffer.cpp
  Obis.cpp
  Options.cpp
  Reading.cpp
  exception.cpp
  local.cpp 
  MeterMap.cpp
  )

add_library(vz ${libvz_srcs})

add_executable(vzlogger ${vzlogger_srcs})

target_link_libraries(vzlogger proto vz vz-api)
target_link_libraries(vzlogger ${JSON_LIBRARY})
target_link_libraries(vzlogger ${SML_LIBRARY})
target_link_libraries(vzlogger ${MICROHTTPD_LIBRARY})
target_link_libraries(vzlogger pthread m ${LIBUUID})
target_link_libraries(vzlogger dl)
if( TARGET )
  if( ${TARGET} STREQUAL "ar71xx")
    set_target_properties(vzlogger PROPERTIES LINK_FLAGS "-static")
    target_link_libraries(vzlogger ${OPENWRT_TARGET_DIR}/usr/lib/libssl.a)
    target_link_libraries(vzlogger ${OPENWRT_TARGET_DIR}/usr/lib/libcrypto.a)
    target_link_libraries(vzlogger ${OPENWRT_TARGET_DIR}/usr/lib/libz.a)
    target_link_libraries(vzlogger dl)
  endif( ${TARGET} STREQUAL "ar71xx")
endif( TARGET )
target_link_libraries(vzlogger ${CURL_STATIC_LIBRARIES} ${CURL_LIBRARIES} ${GNUTLS_LIBRARIES})

# add programs to the install target 
INSTALL(PROGRAMS 
  ${CMAKE_CURRENT_BINARY_DIR}/vzlogger
  DESTINATION bin)
