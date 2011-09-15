AM_CFLAGS = -Wall -D_REENTRANT -std=gnu99
AM_CPPFLAGS = -I$(top_srcdir)/include
AM_LDFLAGS =

VERSION = 1:0:0

lib_LTLIBRARIES = libmeter.la libobis.la

libmeter_la_SOURCES = meter.c d0.c s0.c random.c onewire.c ltqnorm.c
libmeter_la_LDFLAGS = -version-info $(VERSION)

libobis_la_SOURCES = obis.c
libobis_la_LDFLAGS = -version-info $(VERSION)

# SML support
####################################################################
if SML_SUPPORT
libmeter_la_SOURCES += sml.c
AM_CFLAGS += $(DEPS_SML_CFLAGS)
endif
