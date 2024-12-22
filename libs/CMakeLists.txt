
message(STATUS "TGE building libs ...")

# add_subdirectory(mbedtls)
# add_subdirectory(lwip)

add_subdirectory(json-c)
add_subdirectory(EmonLib)
add_subdirectory(HC-SR04)
add_subdirectory(pico-onewire)

# TGE Does not work like that: add_definitions(-DBUILD_SHARED_LIBS=OFF)

