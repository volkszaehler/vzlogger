#include "gtest/gtest.h"
#include "Obis.hpp"
#include "VZException.hpp"

// dirty hack until we find a better solution:
// (already in MeterD0.cpp) #include "../src/Obis.cpp"

TEST(Obis, Obis_basic) {
	// empty Obis constructor
	Obis o1;
	ASSERT_TRUE(o1.isNull());
	ASSERT_TRUE(!o1.isManufacturerSpecific());

	// 2nd constructor:
	Obis o2(0x1, 0x2, 0x3, 0x4, 0x5, 0x6);
	ASSERT_EQ(o2, o2); // check comparison op.
	// 3rd constructor:
	Obis o3("1-2:3.4.5*6");
	ASSERT_EQ(o2, o3);
}

TEST(Obis, Obis_strparsing) {
	Obis o1(0x1, 0x1, 97, 97, 0xff, 0xff); // 97 = SC_F
	Obis o2("1-1:F.F");
	ASSERT_EQ(o1, o2);

	Obis o3(0x1, 0x1, 96, 98, 0xff, 0xff); // 96 = SC_C, 98 = SC_L
	Obis o4("1-1:C.L");
	ASSERT_EQ(o3, o4);

	Obis o5(0x1, 0x1, 96, 99, 0xff, 0xff); // 96 = SC_C, 99 = SC_P
	Obis o6("1-1:C.P");
	ASSERT_EQ(o5, o6);

	ASSERT_THROW(Obis o7("1-1:x:y"), vz::VZException);
	Obis o8("power-l1");
	ASSERT_EQ(Obis("1-0:21.7"), o8);

}

