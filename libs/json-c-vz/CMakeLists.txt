
message(STATUS "TGE building json-c lib into ${CMAKE_CURRENT_BINARY_DIR} ...")

if(NOT DEFINED JSONC_DIR)
    set(JSONC_DIR ${CMAKE_SOURCE_DIR})
endif()

set(src_jsonc
    arraylist.c
    debug.c
    json_c_version.c
    json_object.c
    json_object_iterator.c
    json_tokener.c
    json_util.c
    libjson.c
    linkhash.c
    printbuf.c
    random_seed.c
    strerror_override.c
)

add_library(json-c STATIC ${src_jsonc})

if(VZ_BUILD_ON_PICO)
  target_compile_options(json-c PUBLIC -DVZ_PICO)
endif(VZ_BUILD_ON_PICO)
