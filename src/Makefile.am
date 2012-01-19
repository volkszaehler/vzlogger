AM_CFLAGS = -Wall -D_REENTRANT -std=gnu99
AM_CPPFLAGS = -I$(top_srcdir)/include
AM_LDFLAGS =

VERSION = 1:0:0

lib_LIBRARIES = libmtr.a

libmtr_a_SOURCES = meter.c ltqnorm.c obis.c options.c reading.c
libmtr_a_SOURCES += \
	protocols/s0.c \
	protocols/d0.c \
	protocols/fluksov2.c \
	protocols/file.c \
	protocols/exec.c \
	protocols/random.c

libmtr_a_LDFLAGS = -version-info $(VERSION)

# SML support
####################################################################
if SML_SUPPORT
libmtr_a_SOURCES += protocols/sml.c
AM_CFLAGS += $(DEPS_SML_CFLAGS)
endif
