# -*- mode: cmake; -*-

function(create_dependencies FILE DEPENDENCIES)
  execute_process(COMMAND objdump -p ${FILE}
    OUTPUT_VARIABLE _result
    )
  string(REPLACE "\n" ";" SEXY_LIST ${_result})

  foreach(_line ${SEXY_LIST})
    if( ${_line} MATCHES "NEEDED ")
      message(">>${_line}<<")
      string(REGEX REPLACE "^.*NEEDED[ ]+(.*)" "\\1" _libname ${_line})
      execute_process(COMMAND dpkg -S ${_libname}
	OUTPUT_VARIABLE _result
	)
      string(REPLACE "\n" ";" PKG_LIST ${_result})
      foreach(_line2 ${PKG_LIST})
	string(REGEX REPLACE ":.*" "" _pkg ${_line2})
	list(APPEND _DEPENDENCIES ${_pkg})
      endforeach()      
    endif()
  endforeach()

  list(REMOVE_DUPLICATES _DEPENDENCIES)
  set(_depend "")
  set(_prefix "")
  foreach(_item ${_DEPENDENCIES})
    set(_depend "${_depend}${_prefix}${_item}")
    set(_prefix ",")
  endforeach()

  #string(REGEX REPLACE ";" " ," _depend ${_DEPENDENCIES})

  message("Dep: '${_depend}'")

  set(${DEPENDENCIES} ${_depend} PARENT_SCOPE)
endfunction()
