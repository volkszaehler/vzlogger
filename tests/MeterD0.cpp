#include "gtest/gtest.h"
#include "Options.hpp"
#include "protocols/MeterD0.hpp"

// this is a dirty hack. we should think about better ways/rules to link against the
// test objects.
// e.g. single files in the tests directory that just include the real file. This way one
// will avoid multiple includes of the same file in a different test case
// (or linking against the real file from the CMakeLists.txt)

#include "../src/protocols/MeterD0.cpp"
#include "../src/Options.cpp"
#include "../src/Obis.cpp"
#include "../src/Reading.cpp"

void print(log_level_t l, char const*s1, char const*s2, ...)
{
	if (l!= log_debug)
	{
		fprintf(stderr, "\n  %s:", s2);
		va_list argp;
		va_start(argp, s2);
		vfprintf(stderr, s1, argp);
		va_end(argp);
		fprintf(stderr, "\n");
	}
}

int writes(int fd, const char *str)
{
	EXPECT_NE((char*)0, str);
	int len = strlen(str);
	return write(fd, str, len);
}

TEST(MeterD0, HagerEHZ_basic) {
	char tempfilename[L_tmpnam+1];
	ASSERT_NE(tmpnam_r(tempfilename), (char*)0);
	std::list<Option> options;
	options.push_back(Option("device", tempfilename));
	MeterD0 m(options);
	ASSERT_STREQ(m.device(), tempfilename) << "devicename not eq " << tempfilename;
	ASSERT_EQ(0, mkfifo(tempfilename, S_IRUSR|S_IWUSR));
	int fd = open(tempfilename, O_RDWR);
	ASSERT_NE(fd, -1);
	ASSERT_EQ(SUCCESS, m.open());

	// now we can simulate some input by simply writing into fd
	std::vector<Reading> rds;
	rds.resize(10);

	// write two good data set
	writes(fd, "/HAG5eHZ010C_EHZ1vA02\r\n");
	writes(fd, "1-0:1.8.0*255(000001.2963)\r\n"); // works only with \r\n error (see ack handling)
	writes(fd, "!\n");
	writes(fd, "/HAG5eHZ010C_EHZ1vA02\r\n");
	writes(fd, "1-0:1.7.0*255(000001.2964)\r\n");
	writes(fd, "1-0:1.9.0*255(000001.2965)\r\n");
	writes(fd, "!\n");

	// now read two readings:
	EXPECT_EQ(1, m.read(rds, 10));
	// check obis data:
	ReadingIdentifier *p = rds[0].identifier().get();
	double value = rds[0].value();
	EXPECT_EQ(1.2963, value);
	ObisIdentifier *o = dynamic_cast<ObisIdentifier*>(p);
	ASSERT_NE((ObisIdentifier*)0, o);
	EXPECT_TRUE(Obis(1, 0, 1, 8, 0, 255)==(o->obis()));

	EXPECT_EQ(2, m.read(rds, 2));
	o = dynamic_cast<ObisIdentifier*>(rds[0].identifier().get());
	ASSERT_NE((ObisIdentifier*)0, o);
	value = rds[0].value();
	EXPECT_EQ(1.2964, value);
	EXPECT_TRUE(Obis(1, 0, 1, 7, 0, 255)==(o->obis()));

	o = dynamic_cast<ObisIdentifier*>(rds[1].identifier().get());
	ASSERT_NE((ObisIdentifier*)0, o);
	value = rds[1].value();
	EXPECT_EQ(1.2965, value);
	EXPECT_TRUE(Obis(1, 0, 1, 9, 0, 255)==(o->obis()));

	EXPECT_EQ(0, m.close());

	EXPECT_EQ(0, close(fd));
	EXPECT_EQ(0, unlink(tempfilename));
}

TEST(MeterD0, HagerEHZ_waitsync) {
	char tempfilename[L_tmpnam+1];
	char strend[5] = "end\0";
	ASSERT_NE(tmpnam_r(tempfilename), (char*)0);
	std::list<Option> options;
	options.push_back(Option("device", tempfilename));
	options.push_back(Option("wait_sync", strend));
	MeterD0 m(options);
	ASSERT_STREQ(m.device(), tempfilename) << "devicename not eq " << tempfilename;
	ASSERT_EQ(0, mkfifo(tempfilename, S_IRUSR|S_IWUSR));
	int fd = open(tempfilename, O_RDWR);
	ASSERT_NE(fd, -1);
	ASSERT_EQ(SUCCESS, m.open());

	// now we can simulate some input by simply writing into fd
	std::vector<Reading> rds;
	rds.resize(10);

	// write one good data set: (two as wait_sync end skips the first one!)
	writes(fd, "HAG5eHZ010C_EHZ1vA02\r\n"); // this is invalid as the first char is missing but that's what wait_sync is for
	writes(fd, "1-0:1.8.0*255(000001.2963)\r\n"); // works only with \r\n error (see ack handling)
	writes(fd, "!\n");
	writes(fd, "/HAG5eHZ010C_EHZ1vA02\r\n");
	writes(fd, "2-1:2.3.4*255(999999.9999)\r\n");
	writes(fd, "!\n");

	EXPECT_EQ(1, m.read(rds, 10));
	// check obis data:
	ReadingIdentifier *p = rds[0].identifier().get();
	double value = rds[0].value();
	EXPECT_EQ(999999.9999, value);
	ObisIdentifier *o = dynamic_cast<ObisIdentifier*>(p);
	ASSERT_NE((ObisIdentifier*)0, o);
	EXPECT_TRUE(Obis(2, 1, 2, 3, 4, 255)==(o->obis()));


	EXPECT_EQ(0, m.close());

	EXPECT_EQ(0, close(fd));
	EXPECT_EQ(0, unlink(tempfilename));
}

TEST(MeterD0, LandisGyr_basic) {
	char tempfilename[L_tmpnam+1];
	char str_pullseq[12] = "2f3f210d0a";
	ASSERT_NE(tmpnam_r(tempfilename), (char*)0);
	std::list<Option> options;
	options.push_back(Option("device", tempfilename));
	options.push_back(Option("pullseq", str_pullseq));
	MeterD0 m(options);
	ASSERT_STREQ(m.device(), tempfilename) << "devicename not eq " << tempfilename;
	ASSERT_EQ(0, mkfifo(tempfilename, S_IRUSR|S_IWUSR));
	int fd = open(tempfilename, O_RDWR);
	ASSERT_NE(fd, -1);
	ASSERT_EQ(SUCCESS, m.open());

	// now we can simulate some input by simply writing into fd
	std::vector<Reading> rds;
	rds.resize(10);

	// write one good data set
	/*
	/?!                 <-- Wiederholung der gesendeten Pullsequenz

	/LGZ52ZMD120APt.G03 <-- Antwort mit Herstellerkennung, mögliche Baudrate und Typ
	F.F(00000000)       <-- Fehlercode
	0.0.0( 20000)       <-- Zählernummer
	1.8.1(001846.0*kWh) <-- Hochtarif, bzw. einziger Tarif: Zählerstand Energielieferung
	1.8.2(000000.0*kWh) <-- Niedertarif: Zählerstand Energielieferung
	2.8.1(004329.6*kWh) <-- Hochtarif, bzw. einziger Tarif: Zählerstand Energieeinspeisung
	2.8.2(000000.0*kWh) <-- Niedertarif: Zählerstand Energieeinspeisung
	1.8.0(001846.0*kWh) <-- Summe Zählerstand Energielieferung
	2.8.0(004329.6*kWh) <-- Summe Zählerstand Energieeinspeisung
	!                   <-- Endesequenz
	*/
	// without splitting the MeterD0::read source/logic (or multithreading) we can't
	// wait for the pullseq before we put the data. Thus we just put the data in the buffer
	// and check afterwards whether a pullseq was send as well.


	writes(fd, "/?!\r\n/LGZ52ZMD120APt.G03\r\n");
	writes(fd, "F.F(00000000)\r\n"); // works only with \r\n error (see ack handling)
	writes(fd, "0.0.0( 20000)\r\n");
	writes(fd, "1.8.1(001846.0*kWh)\r\n");
	writes(fd, "1.8.2(000000.0*kWh)\r\n");
	writes(fd, "2.8.1(004329.6*kWh)\r\n");
	writes(fd, "2.8.2(000000.0*kWh)\r\n");
	writes(fd, "1.8.0(001846.0*kWh)\r\n");
	writes(fd, "2.8.0(004329.6*kWh)\r\n");
	writes(fd, "!");

	// now read one readings

	EXPECT_EQ(6, m.read(rds, 10));
	// check whether pullseq is remaining in the fifo:
	char buf[100];
	ssize_t len = read(fd, buf, sizeof(buf));
	ASSERT_EQ((ssize_t)strlen("/?!\r\n"), len) << "buf=[" << buf << "]";

	// check obis data:
	ReadingIdentifier *p = rds[0].identifier().get();
	double value = rds[0].value();
	EXPECT_EQ(1846.0, value);
	ObisIdentifier *o = dynamic_cast<ObisIdentifier*>(p);
	ASSERT_NE((ObisIdentifier*)0, o);
	EXPECT_TRUE(Obis(0xff, 0xff, 1, 8, 1, 255)==(o->obis()));

	o = dynamic_cast<ObisIdentifier*>(rds[1].identifier().get());
	ASSERT_NE((ObisIdentifier*)0, o);
	value = rds[1].value();
	EXPECT_EQ(0.0, value);
	EXPECT_TRUE(Obis(0xff, 0xff, 1, 8, 2, 0xff)==(o->obis()));

	o = dynamic_cast<ObisIdentifier*>(rds[2].identifier().get());
	ASSERT_NE((ObisIdentifier*)0, o);
	value = rds[2].value();
	EXPECT_EQ(4329.6, value);
	EXPECT_TRUE(Obis(0xff, 0xff, 2, 8, 1, 0xff)==(o->obis()));

	EXPECT_EQ(0, m.close());

	EXPECT_EQ(0, close(fd));
	EXPECT_EQ(0, unlink(tempfilename));
}

