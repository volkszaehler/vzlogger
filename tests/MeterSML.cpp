#include <cmath>
#include <fcntl.h>
#include <stdio.h>

#include "gtest/gtest.h"
#include <Options.hpp>
#include <protocols/MeterSML.hpp>

int writes_hex(int fd, const char *str); // impl. in MeterD0.cpp

TEST(MeterSML, EMH_basic) {
	using std::fabs;

	char tempfilename[L_tmpnam + 1];
	ASSERT_NE(tmpnam(tempfilename), (char *)0);
	std::list<Option> options;
	options.push_back(Option("device", tempfilename));
	MeterSML m(options);
	ASSERT_STREQ(m.device(), tempfilename) << "devicename not eq " << tempfilename;
	ASSERT_EQ(0, mkfifo(tempfilename, S_IRUSR | S_IWUSR));
	int fd = open(tempfilename, O_RDWR);
	ASSERT_NE(-1, fd);
	ASSERT_NE(-1, m.open());

	// now we can simulate some input by simply writing into fd
	std::vector<Reading> rds;
	rds.resize(10);

	// write one good data set
	writes_hex(
		fd, "1B1B1B1B010101017607003600001AFA6200620072630101760101070036044808FE093032323830383136"
			"01016331ED007607003600001AFB62006200726307017701093032323830383136017262016504487D8976"
			"77078181C78203FF0101010104454D480177070100000000FF010101010930323238303831360177070100"
			"010801FF63018001621E52FF560008D1CF1B0177070100010802FF63018001621E52FF560000004E9C0177"
			"0700006001FFFF010101010B303030323238303831360177070100010700FF0101621B52FF550000007001"
			"010163D201007607003600001AFC6200620072630201710163077A00001B1B1B1B1A019D37");
	/*
	 * manual parsing of input data:
	 * 1B1B1B1B
	 * 01010101
	 * 76 TL Field list of.., len = 6
	 *  07 1. list element, TL Field : octet string, len 6 (why not 7?)
	 *   003600001AFA  =transaction id
	 *  62 00 2nd list element, TL field: u8, val=0 =group id
	 *  62 00 3rd list element, TL field: u8, val=0 =abort on error
	 *  72 4th list element, TL list of, len = 2 =message body
	 *   630101 1st elem, TL u16, val=101 =tag ("public open req")
	 *   76 2nd elem, TL list of, len = 6
	 *    01 opt codepage
	 *    01  opt clientid
	 *    07 str, len 6= 0036044808FE reqFileId
	 *    09 3032323830383136 serverId
	 *    01 opt time
	 *    01 opt version
	 *  63 5th elem, u16, val = 31ED =crc16
	 *  00 6th elem EndOfSMLMsg
	 * 76 list of, len=6
	 *  07 tl octet string, len 6 = 003600001AFB = transaction id
	 *  62 u8, 00 =groupid
	 *  62 u8, 00 =abort on error
	 *  72 list, len=2 = message body
	 *   63 0701 =tag (get list response)
	 *   77 = message body data list with 7 elements
	 *    01 opt clientid
	 *    09 str, len 8 =3032323830383136 serverId
	 *    01 opt listname
	 *    72 actSensorTime
	 *     62 u8, val=01
	 *     65 u32, val=04487D89
	 *    76 valList
	 *     77
	 *      07 str, l 6 =8181C78203FF objName 129-129:xC7... -> error code
	 *      01 opt status
	 *      01 opt valTime
	 *      01 opt unit
	 *      01 opt scaler
	 *      04 str, l3 =454D48 value
	 *      01 opt valueSignature
	 *     77
	 *      07 str, l 6 =0100000000FF objName 1-0:0.0.0*FF
	 *      01
	 *      01
	 *      01
	 *      01
	 *      09 str, l 8 =3032323830383136 "02280816" Eigentumsnr?
	 *      01
	 *     77
	 *      07 str, l 6 =0100010801FF objName 1-0:1.8.1*FF
	 *      63 u16 =0180 status
	 *      01
	 *      62 u8, =1E unit -> Wh
	 *      52 s8, =FF scaler (-> 10⁻1)
	 *      56 signed int len 5 byte 0008D1CF1B
	 *      01
	 *     77
	 *      07 str l 6 =0100010802FF objName 1-0:1.8.2*FF
	 *      63 u16 =0180
	 *      01
	 *      62 u8 =1E
	 *      52 s8 = FF
	 *      56 s int, len 5 byte 0000004E9C
	 *      01
	 *     77
	 *      07 str l 6 =00006001FFFF 0-0:96.1.FF*FF
	 *      01
	 *      01
	 *      01
	 *      01
	 *      0B str l 10 = 30303032323830383136 "0002280816"
	 *      01
	 *     77
	 *      07 str l 6 =0100010700FF objName 1-0:1.7.0*FF
	 *      01
	 *      01
	 *      62 u8 = 1B unit
	 *      52 s8 = FF scaler-> 10⁻1
	 *      55 s int32 = 00000070 -> 112 -> 11.2
	 *      01
	 *    01 opt listSignature
	 *    01 opt actGatewayTime
	 *   63 u16 =D201 crc16
	 *   00 EndOfSMLMsg
	 *  76
	 *   07 str l 6 = 003600001AFC
	 *   62 u8 = 00
	 *   62 u8 = 00
	 *   72
	 *    63 u16 = 0201
	 *    71
	 *     01
	 *   63 u16 = 077A crc
	 *   00 EndOfSMLMsg
	 *  00
	 *  1B1B1B1B
	 *  1A019D37
	 */
	// now read 3 readings: (currently the string values get's ignored. See MeterSML::_parse
	EXPECT_EQ(3, m.read(rds, 3));
	// check obis data:
	ReadingIdentifier *p = rds[0].identifier().get();
	double value = rds[0].value();
	EXPECT_LE(fabs(14796777.1 - value), 0.1);
	ObisIdentifier *o = dynamic_cast<ObisIdentifier *>(p);
	ASSERT_NE((ObisIdentifier *)0, o);
	EXPECT_TRUE(Obis(1, 0, 1, 8, 1, 255) == (o->obis()));

	o = dynamic_cast<ObisIdentifier *>(rds[1].identifier().get());
	ASSERT_NE((ObisIdentifier *)0, o);
	value = rds[1].value();
	EXPECT_EQ(2012.4, value);
	EXPECT_TRUE(Obis(1, 0, 1, 8, 2, 255) == (o->obis()));

	o = dynamic_cast<ObisIdentifier *>(rds[2].identifier().get());
	ASSERT_NE((ObisIdentifier *)0, o);
	value = rds[2].value();
	EXPECT_LE(fabs(11.2 - value), 0.1);
	EXPECT_TRUE(Obis(1, 0, 1, 7, 0, 255) == (o->obis()));

	EXPECT_EQ(0, m.close());

	EXPECT_EQ(0, close(fd));
	EXPECT_EQ(0, unlink(tempfilename));
}
