#include "gtest/gtest.h"
#include "Options.hpp"
#include "protocols/MeterD0.hpp"

// this is a dirty hack. we should think about better ways/rules to link against the
// test objects.
#include "../src/protocols/MeterD0.cpp"
#include "../src/Options.cpp"
#include "../src/Obis.cpp"
#include "../src/Reading.cpp"

void print(log_level_t, char const*s1, char const*s2, ...)
{
	va_list argp;
	va_start(argp, s2);
	vfprintf(stderr, s1, argp);
	va_end(argp);
	fprintf(stderr, " << %s\n", s2);
}

int writes(int fd, const char *str)
{
	int len = strlen(str);
	return write(fd, str, len);
}

TEST(MeterD0, InitTest) {
	char tempfilename[L_tmpnam+1];
	ASSERT_NE(tmpnam_r(tempfilename), (char*)0);
	std::list<Option> options;
	options.push_back(Option("device", tempfilename));
	options.push_back(Option("wait_sync", "end"));
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
	writes(fd, "/HAG5eHZ010C_EHZ1vA02\r\n");
	writes(fd, "1-0:1.8.0*255(000001.2963)\r\n"); // works only with \r\n error (see ack handling)
	writes(fd, "!\n");
	writes(fd, "/HAG5eHZ010C_EHZ1vA02\r\n");
	writes(fd, "1-0:1.8.0*255(000001.2963)\r\n");
	writes(fd, "!\n");

	EXPECT_EQ(1, m.read(rds, 10));

	EXPECT_EQ(0, m.close());

	EXPECT_EQ(0, close(fd));
	EXPECT_EQ(0, unlink(tempfilename));
}

