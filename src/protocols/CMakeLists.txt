# -*- mode: cmake; -*-

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

set(proto_srcs
  MeterS0.cpp ../../include/protocols/MeterS0.hpp
  MeterD0.cpp ../../include/protocols/MeterD0.hpp
  ${sml_srcs}
  MeterFluksoV2.cpp
  ${ocr_srcs}
  MeterFile.cpp
  MeterExec.cpp
  MeterRandom.cpp
  MeterW1therm.cpp ../../include/protocols/MeterW1therm.hpp
  ${oms_srcs}
  MeterChinaDC.cpp ../../include/protocols/MeterChinaDC.hpp
)

add_library(proto ${proto_srcs})
install(TARGETS proto
	LIBRARY DESTINATION lib COMPONENT libraries
	ARCHIVE DESTINATION lib/static COMPONENT libraries)
