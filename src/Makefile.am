AM_CFLAGS = -Wall -D_REENTRANT -std=gnu99 $(DEPS_VZ_CFLAGS)
AM_CPPFLAGS = -I $(top_srcdir)/include
AM_LDFLAGS =

bin_PROGRAMS = vzlogger

vzlogger_SOURCES = test.cpp buffer.cpp reading.cpp obis.cpp exception.cpp

#vzlogger_SOURCES = vzlogger.c channel.c api.c config.c threads.c buffer.c
#vzlogger_SOURCES += meter.c ltqnorm.c obis.c options.c reading.c

# Protocols (add your own here)
#vzlogger_SOURCES += \
#	protocols/s0.c \
#	protocols/d0.c \
#	protocols/fluksov2.c \
#	protocols/file.c \
#	protocols/exec.c \
#	protocols/random.c

vzlogger_LDADD =
vzlogger_LDFLAGS = -lpthread -lm $(DEPS_VZ_LIBS)

# SML support
####################################################################
if SML_SUPPORT
#vzlogger_SOURCES += protocols/sml.c
vzlogger_LDADD += $(DEPS_SML_LIBS)
AM_CFLAGS += $(DEPS_SML_CFLAGS)
endif

# local interface support
####################################################################
if LOCAL_SUPPORT
#vzlogger_SOURCES += local.c
vzlogger_LDADD += $(DEPS_LOCAL_LIBS)
AM_CFLAGS += $(DEPS_LOCAL_CFLAGS)
endif
