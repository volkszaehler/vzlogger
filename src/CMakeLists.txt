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
else(ENABLE_LOCAL)
  set(local_srcs "")
endif(ENABLE_LOCAL)


set(vzlogger_srcs
  vzlogger.cpp 
  ltqnorm.cpp
  meter.cpp
)

set(libvz_srcs 
  channel.cpp
  config.cpp
  threads.cpp
  buffer.cpp
  obis.cpp
  options.cpp
  reading.cpp
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
target_link_libraries(vzlogger pthread m uuid)
target_link_libraries(vzlogger ${CURL_STATIC_LIBRARIES} ${CURL_LIBRARY})
