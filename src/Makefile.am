AM_CFLAGS = -Wall -D_REENTRANT -std=gnu99
AM_CPPFLAGS = -I$(top_srcdir)/include
AM_LDFLAGS =

VERSION = 1:0:0

lib_LIBRARIES = libmtr.a

libmtr_a_SOURCES = meter.c d0.c s0.c random.c file.c exec.c ltqnorm.c obis.c options.c
libmtr_a_LDFLAGS = -version-info $(VERSION)

# SML support
####################################################################
if SML_SUPPORT
libmtr_a_SOURCES += sml.c
AM_CFLAGS += $(DEPS_SML_CFLAGS)
endif
