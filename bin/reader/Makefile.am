# what flags you want to pass to the C compiler & linker
AM_CFLAGS = -Wall -D_REENTRANT -std=gnu99 $(DEPS_CFLAGS)
AM_LDFLAGS =

# SML
####################################################################
if SML_SUPPORT
bin_PROGRAMS = smlreader
smlreader_SOURCES = smlreader.c
smlreader_LDADD = -lm $(DEPS_SML_LIBS)
smlreader_CFLAGS = $(AM_CFLAGS) $(DEPS_SML_CFLAGS)
endif

