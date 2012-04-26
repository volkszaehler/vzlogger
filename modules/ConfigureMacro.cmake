# -*- mode: cmake; -*-

macro(myconfigure_file IN OUT)

  set(_tmpfile "${OUT}.tmp")
  configure_file(${IN} ${_tmpfile})

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_tmpfile} ${OUT}
    )
endmacro()
