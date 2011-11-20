AM_CFLAGS = -Wall -D_REENTRANT -std=gnu99 $(DEPS_VZ_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/include -Iinclude

bin_PROGRAMS = vzlogger
vzlogger_SOURCES = src/vzlogger.c src/channel.c src/api.c src/config.c src/threads.c src/buffer.c
vzlogger_LDADD = $(top_srcdir)/src/libmtr.a
vzlogger_LDFLAGS = -lpthread -lm $(DEPS_VZ_LIBS)

# local interface support
####################################################################
if LOCAL_SUPPORT
vzlogger_SOURCES += src/local.c
vzlogger_LDADD += $(DEPS_LOCAL_LIBS)
AM_CFLAGS += $(DEPS_LOCAL_CFLAGS)
endif

# SML support
####################################################################
if SML_SUPPORT
vzlogger_LDADD += $(DEPS_SML_LIBS)
AM_CFLAGS += $(DEPS_SML_CFLAGS)
endif
