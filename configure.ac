#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])

# update version here!
AC_INIT([vzlogger], [0.3.3], [http://bugs.volkszaehler.org])
AM_INIT_AUTOMAKE([vzlogger], [0.3.3])

AC_CONFIG_SRCDIR([src/Meter.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
	Makefile
	docs/Makefile
	src/Makefile
])

# Checks for programs.
AC_PROG_CXX

# We use per target compiler flags
AM_PROG_CC_C_O

# Link libraries
AC_PROG_RANLIB

# Checks for libraries.
PKG_CHECK_MODULES([DEPS_VZ], [json >= 0.9 libcurl >= 7.19 openssl ])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stddef.h stdint.h stdlib.h string.h sys/time.h termios.h unistd.h getopt.h signal.h pthread.h])

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
    [AS_HELP_STRING([--enable-sml], [enable support for smart messaging language (def=yes)])],
    [sml=$enableval],
    [sml=yes]
)

AM_CONDITIONAL([SML_SUPPORT], [test x"$sml" = x"yes"])
if test x"$sml" = x"yes"; then
    AC_DEFINE([SML_SUPPORT], [], [Smart Messaging Language])
    PKG_CHECK_MODULES([DEPS_SML], [sml >= 0.1])
fi

# local interface support
AC_ARG_ENABLE(
    [local], 
    [AS_HELP_STRING([--enable-local], [enable support for local HTTPd (def=yes)])],
    [local=$enableval],
    [local=yes]
)

AM_CONDITIONAL([LOCAL_SUPPORT], [test x"$local" = x"yes"])
if test x"$local" = x"yes"; then
    AC_DEFINE([LOCAL_SUPPORT], [], [Local interface])
    PKG_CHECK_MODULES([DEPS_LOCAL], [libmicrohttpd >= 0.4.6])
fi

# build reader binary
AC_ARG_WITH(
    [reader],
    [AS_HELP_STRING([--with-reader], [compile reader to for testing your meters (def=yes)])],
    [reader=$withval]
    [reader=yes]
)

AM_CONDITIONAL([READER_BUILD], [test x"$reader" = x"yes"])

# debug compilation support
AC_ARG_ENABLE(
    [debug],
    [AS_HELP_STRING([--enable-debug], [enable debug data generation (def=no)])],
    [debug=$enableval],
    [debug=no]
)

if test x"$debug" = x"yes"; then
    AC_DEFINE([DEBUG], [], [Debug Mode])
    AM_CXXFLAGS="$AM_CXXFLAGS -g -Wall -Werror -Wno-uninitialized -O0 -fno-omit-frame-pointer -Woverloaded-virtual -Wno-system-headers"
else
    AM_CXXFLAGS="$AM_CXXFLAGS -O3"
fi

AC_OUTPUT
