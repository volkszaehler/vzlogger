#include "gtest/gtest.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "Options.hpp"
#include "protocols/MeterFile.hpp"

// this is a dirty hack. we should think about better ways/rules to link against the
// test objects.
// e.g. single files in the tests directory that just include the real file. This way one
// will avoid multiple includes of the same file in a different test case
// (or linking against the real file from the CMakeLists.txt)

// #include "../src/protocols/MeterFile.cpp"

int writes(int fd, const char *str);

TEST(MeterFile, basic) {
	char tempfilename[L_tmpnam + 1];
	ASSERT_NE(tmpnam(tempfilename), (char *)0);
	std::list<Option> options;
	options.push_back(Option("path", tempfilename));
	options.push_back(Option("interval", 1));

	// test without format option
	MeterFile m(options);
	ASSERT_STREQ(m.path(), tempfilename) << "devicename not eq " << tempfilename;
	ASSERT_EQ(0, mkfifo(tempfilename, S_IRUSR | S_IWUSR));
	int fd = open(tempfilename, O_RDWR);
	ASSERT_NE(fd, -1);
	ASSERT_EQ(SUCCESS, m.open());

	// now we can simulate some input by simply writing into fd
	std::vector<Reading> rds;
	rds.resize(10);

	// write two good data set
	writes(fd, "32552\r\n");
	writes(fd, "32552.5\r\n");
	//	writes(fd, "32552.6\r\n"); // bug: with 2 only it hangs! Fixed with changing order in while
	//(i<n...) in MeterFile.cpp

	// now read two readings:
	EXPECT_EQ(2, m.read(rds, 2));
	// check obis data:
	ReadingIdentifier *p = rds[0].identifier().get();
	double value = rds[0].value();
	EXPECT_EQ(32552, value);
	StringIdentifier *o = dynamic_cast<StringIdentifier *>(p);
	ASSERT_NE((StringIdentifier *)0, o);
	EXPECT_EQ(StringIdentifier(""), *o);

	value = rds[1].value();
	p = rds[1].identifier().get();
	o = dynamic_cast<StringIdentifier *>(p);
	EXPECT_EQ(32552.5, value);
	ASSERT_NE((StringIdentifier *)0, o);
	EXPECT_EQ(StringIdentifier(""), *o);

	EXPECT_EQ(0, m.close());

	EXPECT_EQ(0, close(fd));
	EXPECT_EQ(0, unlink(tempfilename));
}

TEST(MeterFile, format1) {
	char tempfilename[L_tmpnam + 1];
	ASSERT_NE(tmpnam(tempfilename), (char *)0);
	std::list<Option> options;
	options.push_back(Option("path", tempfilename));
	options.push_back(Option("format", (char *)"$v"));
	options.push_back(Option("interval", 1));

	MeterFile m(options);
	ASSERT_STREQ(m.path(), tempfilename) << "devicename not eq " << tempfilename;
	ASSERT_EQ(0, mkfifo(tempfilename, S_IRUSR | S_IWUSR));
	int fd = open(tempfilename, O_RDWR);
	ASSERT_NE(fd, -1);
	ASSERT_EQ(SUCCESS, m.open());

	// now we can simulate some input by simply writing into fd
	std::vector<Reading> rds;
	rds.resize(10);

	// write two good data set
	writes(fd, "32552\r\n");
	writes(fd, "32552.5\r\n");

	// now read two readings:
	EXPECT_EQ(2, m.read(rds, 2));
	// check obis data:
	ReadingIdentifier *p = rds[0].identifier().get();
	double value = rds[0].value();
	EXPECT_EQ(32552, value);
	StringIdentifier *o = dynamic_cast<StringIdentifier *>(p);
	ASSERT_NE((StringIdentifier *)0, o);
	EXPECT_EQ(StringIdentifier("<null>"), *o);

	value = rds[1].value();
	p = rds[1].identifier().get();
	o = dynamic_cast<StringIdentifier *>(p);
	EXPECT_EQ(32552.5, value);
	ASSERT_NE((StringIdentifier *)0, o);
	EXPECT_EQ(StringIdentifier("<null>"), *o);

	EXPECT_EQ(0, m.close());

	EXPECT_EQ(0, close(fd));
	EXPECT_EQ(0, unlink(tempfilename));
}

TEST(MeterFile, format2) {
	char tempfilename[L_tmpnam + 1];
	ASSERT_NE(tmpnam(tempfilename), (char *)0);
	std::list<Option> options;
	options.push_back(Option("path", tempfilename));
	options.push_back(Option("format", (char *)"$i : $v"));
	options.push_back(Option("interval", 1));

	MeterFile m(options);
	ASSERT_STREQ(m.path(), tempfilename) << "devicename not eq " << tempfilename;
	ASSERT_EQ(0, mkfifo(tempfilename, S_IRUSR | S_IWUSR));
	int fd = open(tempfilename, O_RDWR);
	ASSERT_NE(fd, -1);
	ASSERT_EQ(SUCCESS, m.open());

	// now we can simulate some input by simply writing into fd
	std::vector<Reading> rds;
	rds.resize(2);

	// write two good data set
	writes(fd, "id1 : 32552\r\n");
	writes(fd, "id2 : 32552.5\r\n");

	// now read two readings:
	EXPECT_EQ(2, m.read(rds, 2));
	// check obis data:
	ReadingIdentifier *p = rds[0].identifier().get();
	double value = rds[0].value();
	EXPECT_EQ(32552, value);
	StringIdentifier *o = dynamic_cast<StringIdentifier *>(p);
	ASSERT_NE((StringIdentifier *)0, o);
	EXPECT_EQ(StringIdentifier("id1"), *o);

	value = rds[1].value();
	p = rds[1].identifier().get();
	o = dynamic_cast<StringIdentifier *>(p);
	EXPECT_EQ(32552.5, value);
	ASSERT_NE((StringIdentifier *)0, o);
	EXPECT_EQ(StringIdentifier("id2"), *o);

	EXPECT_EQ(0, m.close());

	EXPECT_EQ(0, close(fd));
	EXPECT_EQ(0, unlink(tempfilename));
}

TEST(MeterFile, reading_times) {
	Reading r;
	struct timeval t;
	t.tv_sec = 1001;
	t.tv_usec = 1;
	r.time(t);
	ASSERT_EQ(1001000ll, r.time_ms()) << r.time_ms();
	t.tv_usec = 1000;
	r.time(t);
	ASSERT_EQ(1001001ll, r.time_ms()) << r.time_ms();

	r.time_from_double(1002.00001);
	ASSERT_EQ(1002000ll, r.time_ms()) << r.time_ms();
}

TEST(MeterFile, format3) {
	char tempfilename[L_tmpnam + 1];
	ASSERT_NE(tmpnam(tempfilename), (char *)0);
	std::list<Option> options;
	options.push_back(Option("path", tempfilename));
	options.push_back(Option("format", (char *)"$t;$i : $v"));
	options.push_back(Option("interval", 1));

	MeterFile m(options);
	ASSERT_STREQ(m.path(), tempfilename) << "devicename not eq " << tempfilename;
	ASSERT_EQ(0, mkfifo(tempfilename, S_IRUSR | S_IWUSR));
	int fd = open(tempfilename, O_RDWR);
	ASSERT_NE(fd, -1);
	ASSERT_EQ(SUCCESS, m.open());

	// now we can simulate some input by simply writing into fd
	std::vector<Reading> rds;
	rds.resize(2);

	// write two good data set
	writes(fd, "1001.2;id1 : 32552\r\n");
	writes(fd, "1002.5;id2 : 32552.5\r\n");

	// now read two readings:
	EXPECT_EQ(2, m.read(rds, 2));
	// check obis data:
	ReadingIdentifier *p = rds[0].identifier().get();
	double value = rds[0].value();
	EXPECT_EQ(32552, value);
	EXPECT_EQ(1001200ll, rds[0].time_ms()) << rds[0].time_ms();

	StringIdentifier *o = dynamic_cast<StringIdentifier *>(p);
	ASSERT_NE((StringIdentifier *)0, o);
	EXPECT_EQ(StringIdentifier("id1"), *o);

	value = rds[1].value();
	p = rds[1].identifier().get();
	o = dynamic_cast<StringIdentifier *>(p);
	EXPECT_EQ(32552.5, value);
	EXPECT_EQ(1002500ll, rds[1].time_ms());
	ASSERT_NE((StringIdentifier *)0, o);
	EXPECT_EQ(StringIdentifier("id2"), *o);

	EXPECT_EQ(0, m.close());

	EXPECT_EQ(0, close(fd));
	EXPECT_EQ(0, unlink(tempfilename));
}
