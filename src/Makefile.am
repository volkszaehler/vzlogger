AM_CFLAGS = -Wall -D_REENTRANT -std=gnu99
AM_CPPFLAGS = -I$(top_srcdir)/include
AM_LDFLAGS =

lib_LTLIBRARIES = libmeter.la libobis.la

libmeter_la_SOURCES = meter.c d0.c s0.c random.c onewire.c ltqnorm.c
libobis_la_SOURCES = obis.c

# SML support
####################################################################
if SML_SUPPORT
libmeter_la_SOURCES += sml.c
AM_CFLAGS += $(DEPS_SML_CFLAGS)
endif
