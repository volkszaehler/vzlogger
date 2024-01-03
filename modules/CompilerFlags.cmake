# -*- mode: cmake; -*-
if (${CMAKE_BUILD_TYPE} MATCHES "Release")
  add_definitions("-DNDEBUG")
endif (${CMAKE_BUILD_TYPE} MATCHES "Release")

if(NOT WIN32)
  if (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    message(STATUS "using gcc compiler ${CMAKE_CXX_COMPILER_ID}")
    include (CheckCXXSourceCompiles)

    set(CMAKE_CXX_FLAGS "${CXXFLAGS} -W -Wall -Wextra -Werror -Wnon-virtual-dtor -Wno-system-headers")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Winit-self -Wmissing-include-dirs -Wno-pragmas -Wredundant-decls")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -std=c++14")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=redundant-decls")

    # produces a lot of warnings with (at least) boost 1.38:
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wswitch-default -Wfloat-equal")

    set (CMAKE_REQUIRED_FLAGS "-Wno-ignored-qualifiers")
    set (__CXX_FLAG_CHECK_SOURCE "int main () { return 0; }")

    message(STATUS "checking if ${CMAKE_REQUIRED_FLAGS} works")
    CHECK_CXX_SOURCE_COMPILES ("${__CXX_FLAG_CHECK_SOURCE}" W_NO_IGNORED_QUALIFIERS)
    set (CMAKE_REQUIRED_FLAGS)
    if (W_NO_IGNORED_QUALIFIERS)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-qualifiers")
    endif (W_NO_IGNORED_QUALIFIERS)

    # release flags
    # Releases are made with the release build. Optimize code.
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wno-unused-parameter")
    #set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Werror -Wno-non-virtual-dtor")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-redundant-decls")
    #set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wno-non-virtual-dtor")

    # debug flags
    set(CMAKE_CXX_FLAGS_DEBUG
      "-O0 -g -ggdb -fno-omit-frame-pointer -Woverloaded-virtual -Wno-system-headers"
      )
    #    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wunused-variable -Wunused-parameter")
    #    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wunused-function -Wunused")

    # gprof and gcov support
    #set(CMAKE_CXX_FLAGS_PROFILE "-O0 -pg -g -ggdb -Wall")
    #set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Wreturn-type")
    #set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Woverloaded-virtual")
    #set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Wno-system-headers"
    #  CACHE STRING "Flags for Profile build"
    #  )
    #  # gprof and gcov support
    set(CMAKE_CXX_FLAGS_PROFILE "-O0 -pg -g -ggdb -W -Wreturn-type -Wno-shadow -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Wunused-variable -Wunused-parameter")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Wunused-function -Wunused")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Woverloaded-virtual -Wno-system-headers")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -Wno-non-virtual-dtor -pg -fprofile-arcs")
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -ftest-coverage")

    # Experimental
    set(CMAKE_CXX_FLAGS_EXPERIMENTAL "-O0 -pg -g -ggdb -Wall -Werror -W -Wshadow")
    set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wunused-variable")
    set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wunused-parameter")
    set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wunused-function")
    set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wunused -Woverloaded-virtual")
    set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wno-system-headers")
    set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -Wno-non-virtual-dtor")
    set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -pg -fprofile-generate ")
    set(CMAKE_CXX_FLAGS_EXPERIMENTAL "${CMAKE_CXX_FLAGS_EXPERIMENTAL} -fprofile-arcs -ftest-coverage")

    set(CMAKE_C_FLAGS "${CFLAGS} -W -Wall -Wno-system-headers")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -fkeep-inline-functions")

  endif (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")

  if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    message(STATUS "using clang compiler ${CMAKE_CXX_COMPILER_ID}")
    set(CMAKE_CXX_FLAGS "${CXXFLAGS} -W -Wall -Wextra -Werror")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-parentheses")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-constant-logical-operand")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  endif()

  # TODO: we need to check the compiler here, gcc does not know about those flags, is this The Right Thing To Do (TM)?
  if (${CMAKE_CXX_COMPILER_ID} MATCHES "Intel")
    set(CMAKE_CXX_FLAGS "${CXXFLAGS} -wd383 -wd981")
    message(STATUS "compiler: ${CMAKE_CXX_COMPILER_MAJOR}.${CMAKE_CXX_COMPILER_MINOR}")
    if (${CMAKE_CXX_COMPILER_MAJOR} GREATER 9)
      message(STATUS "Warning: adding __aligned__=ignored to the list of definitions")
      add_definitions("-D__aligned__=ignored")
    endif (${CMAKE_CXX_COMPILER_MAJOR} GREATER 9)
  endif (${CMAKE_CXX_COMPILER_ID} MATCHES "Intel")
else(NOT WIN32)
  ## currently notyhing
  ##set( "/NODEFAULTLIB")
  set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} ws2_32.lib rpcrt4.lib")
  set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} ws2_32.lib rpcrt4.lib")
  #set(CMAKE_CXX_FLAGS "/Z7")
endif(NOT WIN32)

# check for specific c++ features:
if(CMAKE_CROSSCOMPILING)
	include(CheckCXXSourceCompiles)
	check_cxx_source_compiles("
		  #include <regex>

		  int main() {
		    return std::regex_match(\"test\", std::regex(\"\^\\\\\\\\s*(//(.*|)|\$)\"));
		  }
	" HAVE_CPP_REGEX )
else(CMAKE_CROSSCOMPILING)
	include(CheckCXXSourceRuns)
	check_cxx_source_runs("
		  #include <regex>

		  int main() {
		    return std::regex_match(\"test\", std::regex(\"\^\\\\\\\\s*(//(.*|)|\$)\"));
		  }
	" HAVE_CPP_REGEX)
endif(CMAKE_CROSSCOMPILING)

