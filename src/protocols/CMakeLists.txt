# -*- mode: cmake; -*-

# SML support
#####################################################################
if( SML_SUPPORT )
  set(sml_srcs MeterSML.cpp)
else( SML_SUPPORT )
  set(sml_srcs "")
endif( SML_SUPPORT )

set(proto_srcs
  MeterS0.cpp
  MeterD0.cpp
  ${sml_srcs}
  MeterFluksoV2.cpp
  MeterFile.cpp
  MeterExec.cpp
  MeterRandom.cpp
)

add_library(proto ${proto_srcs})
