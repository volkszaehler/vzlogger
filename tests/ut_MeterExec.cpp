#include "gtest/gtest.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "Options.hpp"
#include "protocols/MeterExec.hpp"

// this is a dirty hack. we should think about better ways/rules to link against the
// test objects.
// e.g. single files in the tests directory that just include the real file. This way one
// will avoid multiple includes of the same file in a different test case
// (or linking against the real file from the CMakeLists.txt)

// #include "../src/protocols/MeterExec.cpp"

int writes(int fd, const char *str);

TEST(MeterExec, basic) {
	char tempfilename[L_tmpnam + 1];
	ASSERT_NE(tmpnam(tempfilename), (char *)0);
	std::list<Option> options;
	options.push_back(Option("command", tempfilename));

	// test without format option
	MeterExec m(options);
	ASSERT_STREQ(m.command(), tempfilename) << "devicename not eq " << tempfilename;

	EXPECT_EQ(0, m.close());
}

TEST(MeterExec, format1) {
	char tempfilename[L_tmpnam + 1];
	ASSERT_NE(tmpnam(tempfilename), (char *)0);
	std::list<Option> options;
	options.push_back(Option("command", tempfilename));
	options.push_back(Option("format", (char *)"$v"));

	MeterExec m(options);
	// todo check here whether format is correctly parsed
	EXPECT_EQ(0, m.close());
}

TEST(MeterExec, format2) {
	char tempfilename[L_tmpnam + 1];
	ASSERT_NE(tmpnam(tempfilename), (char *)0);
	std::list<Option> options;
	options.push_back(Option("command", tempfilename));
	options.push_back(Option("format", (char *)"$i : $v"));

	MeterExec m(options);
	// todo check here whether format is correctly parsed
	EXPECT_EQ(0, m.close());
}

TEST(MeterExec, format3) {
	char tempfilename[L_tmpnam + 1];
	ASSERT_NE(tmpnam(tempfilename), (char *)0);
	std::list<Option> options;
	options.push_back(Option("command", tempfilename));
	options.push_back(Option("format", (char *)"$t;$i : $v"));

	MeterExec m(options);
	// todo check here whether format is correctly parsed
	EXPECT_EQ(0, m.close());
}
