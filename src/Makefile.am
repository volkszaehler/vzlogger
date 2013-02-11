AM_CFLAGS = -Wall -D_REENTRANT  $(DEPS_VZ_CFLAGS)
#AM_CFLAGS = -Wall -D_REENTRANT  $(DEPS_VZ_CFLAGS)

AM_CPPFLAGS = -I $(top_srcdir)/include -std=c++0x
AM_LDFLAGS =

bin_PROGRAMS = vzlogger

#vzlogger_SOURCES = test.cpp buffer.cpp reading.cpp obis.cpp exception.cpp

vzlogger_SOURCES = vzlogger.cpp Channel.cpp Config_Options.cpp threads.cpp Buffer.cpp
vzlogger_SOURCES += Meter.cpp ltqnorm.cpp Obis.cpp Options.cpp Reading.cpp
vzlogger_SOURCES += exception.cpp local.cpp MeterMap.cpp


# Protocols (add your own here)
vzlogger_SOURCES += \
	protocols/MeterS0.cpp \
	protocols/MeterD0.cpp \
	protocols/MeterSML.cpp \
	protocols/MeterFluksoV2.cpp \
	protocols/MeterFile.cpp \
	protocols/MeterExec.cpp \
	protocols/MeterRandom.cpp

# logger API (add your own here)
vzlogger_SOURCES += \
	api/Volkszaehler.cpp \
	api/MySmartGrid.cpp \
	api/CurlIF.cpp \
	api/CurlCallback.cpp \
	api/CurlResponse.cpp

vzlogger_LDADD =
vzlogger_LDFLAGS = -lpthread -lm -lstdc++ $(DEPS_VZ_LIBS)

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
