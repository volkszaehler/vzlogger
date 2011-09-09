#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([vzlogger], [0.2], [http://bugs.volkszaehler.org])
AM_INIT_AUTOMAKE(vzlogger, 0.2)
AC_CONFIG_SRCDIR([src/meter.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile
		   docs/Makefile
		   src/Makefile
		   bin/reader/Makefile
		   bin/vzlogger/Makefile
		])

# Checks for programs.
AC_PROG_CC

# We want to build a lib
AC_PROG_RANLIB

# We use per target compiler flags
AM_PROG_CC_C_O

# Checks for libraries.
PKG_CHECK_MODULES([DEPS_VZ], [json >= 0.9 libcurl >= 7.21.0])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stddef.h stdint.h stdlib.h string.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_MODE_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([gettimeofday memset sqrt strchr strtol])

# SML support
AC_ARG_ENABLE(
    [sml],
    [AS_HELP_STRING([--enable-sml], [enable support for smart messaging language (def=no)])],
    [sml=true],
    [sml=false]
)

AM_CONDITIONAL([SML_SUPPORT], [test x"$sml" = x"true"])
if test x"$sml" = x"true"; then
    AC_DEFINE([SML_SUPPORT], [], [Smart Messaging Language])
    PKG_CHECK_MODULES([DEPS_SML], [sml >= 0.1])
fi

# local interface support
AC_ARG_ENABLE(
    [local], 
    [AS_HELP_STRING([--enable-local], [enable support for local HTTPd (def=no)])],
    [local=true],
    [local=false]
)

AM_CONDITIONAL([LOCAL_SUPPORT], [test x"$local" = x"true"])
if test x"$local" = x"true"; then
    AC_DEFINE([LOCAL_SUPPORT], [], [Local interface])
    PKG_CHECK_MODULES([DEPS_LOCAL], [libmicrohttpd >= 0.9.3])
fi

# debug compilation support
AC_ARG_ENABLE(
    [debug],
    [AS_HELP_STRING([--enable-debug], [enable debug data generation (def=no)])],
    [debug=true],
    [debug=false]
)

if test x"$debug" = x"true"; then
    AC_DEFINE([DEBUG],[],[Debug Mode])
    AM_CXXFLAGS="$AM_CXXFLAGS -g -Wall -Werror -Wno-uninitialized -O0"
else
    AM_CXXFLAGS="$AM_CXXFLAGS -O3"
fi

AC_OUTPUT
