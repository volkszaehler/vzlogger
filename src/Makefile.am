AM_CFLAGS = -Wall -D_REENTRANT -std=gnu99
AM_CPPFLAGS = -I$(top_srcdir)/include
AM_LDFLAGS =

noinst_LIBRARIES = libmeter.a libobis.a

libmeter_a_SOURCES = meter.c d0.c s0.c random.c onewire.c ltqnorm.c
#libmeter_a_HEADERS = meter.h

libobis_a_SOURCES = obis.c
#libobis_a_HEADERS = obis.h

# SML support
####################################################################
if SML_SUPPORT
libmeter_a_SOURCES += sml.c
libmeter_a_LIBADD = libobis.a
AM_CFLAGS += $(DEPS_SML_CFLAGS)
endif
