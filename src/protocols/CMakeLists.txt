# -*- mode: cmake; -*-

set(SML_SUPPORT ${VZ_USE_METER_SML})
set(OCR_SUPPORT ${VZ_USE_METER_OCR})
set(OCR_TESSERACT_SUPPORT ${VZ_USE_METER_OCRTESSERACT})
set(OMS_SUPPORT ${VZ_USE_METER_OMS})

# SML support
#####################################################################
if( SML_SUPPORT )
  set(sml_srcs MeterSML.cpp)
else( SML_SUPPORT )
  set(sml_srcs "")
endif( SML_SUPPORT )

if( OCR_TESSERACT_SUPPORT )
  set(ocr_srcs MeterOCR.cpp MeterOCRTesseract.cpp)
else ()
    if (OCR_SUPPORT)
        set(ocr_srcs MeterOCR.cpp)
    else ()
        set(ocr_srcs "")
    endif (OCR_SUPPORT)
endif( OCR_TESSERACT_SUPPORT )

if( OMS_SUPPORT )
  set(oms_srcs MeterOMS.cpp ../../include/protocols/MeterOMS.hpp)
else ( OMS_SUPPORT )
  set(oms_srcs "")
endif( OMS_SUPPORT )

if( VZ_USE_METER_D0 )
  set(d0_srcs MeterD0.cpp ../../include/protocols/MeterD0.hpp)
else ( VZ_USE_METER_D0 )
  set(d0_srcs "")
endif( VZ_USE_METER_D0 )

if( VZ_USE_METER_S0 )
  set(s0_srcs MeterS0.cpp ../../include/protocols/MeterS0.hpp)
else ( VZ_USE_METER_S0 )
  set(s0_srcs "")
endif( VZ_USE_METER_S0 )

if( VZ_USE_METER_EXEC )
  set(exec_srcs MeterExec.cpp)
else ( VZ_USE_METER_EXEC )
  set(exec_srcs "")
endif( VZ_USE_METER_EXEC )

if( VZ_USE_METER_FILE )
  set(file_srcs MeterFile.cpp)
else ( VZ_USE_METER_FILE )
  set(file_srcs "")
endif( VZ_USE_METER_FILE )

if( VZ_USE_METER_FLUKSOV2 )
  set(fluksov2_srcs MeterFluksoV2.cpp)
else ( VZ_USE_METER_FLUKSOV2 )
  set(fluksov2_srcs "")
endif( VZ_USE_METER_FLUKSOV2 )

if( VZ_USE_METER_RANDOM )
  set(random_srcs MeterRandom.cpp)
else ( VZ_USE_METER_RANDOM )
  set(random_srcs "")
endif( VZ_USE_METER_RANDOM )

if( VZ_USE_METER_W1THERM )
  set(w1therm_srcs MeterW1therm.cpp ../../include/protocols/MeterW1therm.hpp)
else ( VZ_USE_METER_W1THERM )
  set(w1therm_srcs "")
endif( VZ_USE_METER_W1THERM )

if( VZ_USE_METER_PICO_SCT013 )
  set(pico_sct013_srcs MeterSCT013.cpp ../../include/protocols/MeterSCT013.hpp)
else ( VZ_USE_METER_PICO_SCT013 )
  set(pico_sct013_srcs "")
endif( VZ_USE_METER_PICO_SCT013 )

if( VZ_USE_METER_PICO_ONBOARD_TEMP )
  set(pico_onboardTemp_srcs MeterOnboardTemp.cpp ../../include/protocols/MeterOnboardTemp.hpp)
else ( VZ_USE_METER_PICO_ONBOARD_TEMP )
  set(pico_onboardTemp_srcs "")
endif( VZ_USE_METER_PICO_ONBOARD_TEMP )


set(proto_srcs
  ${s0_srcs}
  ${d0_srcs}
  ${exec_srcs}
  ${file_srcs}
  ${fluksov2_srcs}
  ${random_srcs}
  ${w1therm_srcs}
  ${sml_srcs}
  ${ocr_srcs}
  ${oms_srcs}
  ${pico_sct013_srcs}
  ${pico_onboardTemp_srcs}
)

add_library(proto STATIC ${proto_srcs})

if(VZ_BUILD_ON_PICO)
 target_compile_options(proto PUBLIC -fexceptions -DVZ_PICO)
else(VZ_BUILD_ON_PICO)
  install(TARGETS proto
	  LIBRARY DESTINATION lib COMPONENT libraries
	  ARCHIVE DESTINATION lib/static COMPONENT libraries)
endif(VZ_BUILD_ON_PICO)
