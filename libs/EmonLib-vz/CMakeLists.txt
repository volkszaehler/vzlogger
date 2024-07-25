
message(STATUS "TGE building EmonLib lib into ${CMAKE_CURRENT_BINARY_DIR} ...")

set(src_emonlib
    EmonLib.cpp
    EmonLib_pico.cpp
)

add_library(emonlib STATIC ${src_emonlib})

target_link_libraries(emonlib pico_stdlib hardware_adc pico_runtime)

if(VZ_BUILD_ON_PICO)
  target_compile_options(emonlib PUBLIC -DVZ_PICO)
endif(VZ_BUILD_ON_PICO)
