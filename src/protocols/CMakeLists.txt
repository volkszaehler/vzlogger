# -*- mode: cmake; -*-

# SML support
#####################################################################
if( SML_SUPPORT )
  set(sml_srcs sml.cpp)
else( SML_SUPPORT )
  set(sml_srcs "")
endif( SML_SUPPORT )

set(proto_srcs
  s0.cpp
  d0.cpp
  ${sml_srcs}
  fluksov2.cpp
  file.cpp
  exec.cpp
  random.cpp
)

add_library(proto ${proto_srcs})
