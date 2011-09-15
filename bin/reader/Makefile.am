# what flags you want to pass to the C compiler & linker
AM_CFLAGS = -Wall -D_REENTRANT -std=gnu99 $(DEPS_VZ_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/include -Iinclude
AM_LDFLAGS =

# SML
####################################################################
if SML_SUPPORT
bin_PROGRAMS = smlreader
smlreader_SOURCES = src/smlreader.c
smlreader_LDADD = -lm $(DEPS_SML_LIBS) ../../src/libobis.la
AM_CFLAGS += $(DEPS_SML_CFLAGS)
endif

