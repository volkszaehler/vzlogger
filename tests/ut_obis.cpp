#include "Obis.hpp"
#include "VZException.hpp"
#include "gtest/gtest.h"

// dirty hack until we find a better solution:
// (already in MeterD0.cpp) #include "../src/Obis.cpp"

TEST(Obis, Obis_basic) {
	// empty Obis constructor
	Obis o1;
	ASSERT_TRUE(o1.isAllNotGiven());
	ASSERT_TRUE(!o1.isManufacturerSpecific());

	// 2nd constructor:
	Obis o2(0x1, 0x2, 0x3, 0x4, 0x5, 0x6);
	ASSERT_FALSE(o1 == o2);
	ASSERT_FALSE(o2.isAllNotGiven());
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

TEST(Obis, Obis_extStrParsing) {
	Obis o1(0xff, 0xff, 0x0, 0x0, 0x0, 0xff);
	Obis o2("0.0.0");
	ASSERT_EQ(o1, o2) << o1.toString() << o2.toString();
	ASSERT_EQ(o1.toString(), o2.toString());

	Obis o3(0xff, 0xff, 0x1, 0x2, 0x3, 0xff);
	Obis o4("1.2.3");
	ASSERT_EQ(o3, o4);
	ASSERT_EQ(o3.toString(), o4.toString());

	Obis o5(0xff, 0xff, 0x61, 0x61, 0xff, 0xff);
	Obis o6("F.F");
	ASSERT_EQ(o5, o6);

	Obis o7(0xff, 0xff, 0x0, 0x0, 0x2, 0xff);
	Obis o8("0.0.2");
	ASSERT_EQ(o7, o8);

	// this should throw an exception as hex codes are not valid inside obis codes.
	ASSERT_THROW(Obis o10("1.8.0*FF"), vz::VZException);
	ASSERT_THROW(Obis o10("1.8.0*F1"), vz::VZException);
}

TEST(Obis, Obis_parse_LuG_E350) // all known codes from Landis+Gyr E350
{
	// commented ones are covered by other tests
	// 	F.F       00
	{
		Obis o1(0xff, 0xff, 0, 0, 0xff, 0xff);
		Obis o2("0.0");
		ASSERT_EQ(o1, o2) << o1.toString() << o2.toString();
		ASSERT_EQ(o1.toString(), o2.toString());
	}
	{
		Obis o1(0xff, 0xff, 96, 1, 0, 0xff);
		Obis o2("C.1.0");
		ASSERT_EQ(o1, o2) << o1.toString() << o2.toString();
		ASSERT_EQ(o1.toString(), o2.toString());
	}
	//	C.1.1
	//	1.8.1     003400.447 kWh
	//	1.8.2     005230.688 kWh
	//	2.8.1     000000.000 kWh
	//	2.8.2     000000.842 kWh
	//	1.8.0     008631.135 kWh
	//	2.8.0     000000.842 kWh
	{
		Obis o1(0xff, 0xff, 15, 8, 0, 0xff);
		Obis o2("15.8.0");
		ASSERT_EQ(o1, o2) << o1.toString() << o2.toString();
		ASSERT_EQ(o1.toString(), o2.toString());
	}
	{
		Obis o1(0xff, 0xff, 16, 7, 0xff, 0xff);
		Obis o2("16.7");
		ASSERT_EQ(o1, o2) << o1.toString() << o2.toString();
		ASSERT_EQ(o1.toString(), o2.toString());
	}
	{
		Obis o1(0xff, 0xff, 72, 7, 0xff, 0xff);
		Obis o2("72.7");
		ASSERT_EQ(o1, o2) << o1.toString() << o2.toString();
		ASSERT_EQ(o1.toString(), o2.toString());
	}

	//	32.7      248 V
	//	52.7      248 V
	//	72.7      009 V
	//	31.7      000.03 A
	//	51.7      000.04 A
	//	71.7      000.05 A
	{
		Obis o1(0xff, 0xff, 82, 8, 2, 0xff);
		Obis o2("82.8.2");
		ASSERT_EQ(o1, o2) << o1.toString() << o2.toString();
		ASSERT_EQ(o1.toString(), o2.toString());
	}

	//	82.8.1    0001
	//	0.2.0     M23
	//	C.5.0     0433
}
