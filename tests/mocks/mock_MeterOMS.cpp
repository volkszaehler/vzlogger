#include <gmock/gmock.h>
using ::testing::_;
using ::testing::AtLeast;
using ::testing::Eq;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnRef;

#include <gtest/gtest.h>
using ::testing::Test;

#include "Meter.hpp"
#include "protocols/MeterOMS.hpp"

namespace mock_MeterOMS {

class mock_OMShwif : public MeterOMS::OMSHWif {
  public:
	mock_OMShwif() : returned(0), remain(0), data(0){};
	virtual ~mock_OMShwif() {
		if (data)
			free(data);
	};
	// MOCK_METHOD0(scanW1devices, bool());
	// MOCK_CONST_METHOD0(W1devices, const std::list<std::string> &());
	MOCK_METHOD2(read, ssize_t(void *buf, size_t count));
	MOCK_METHOD2(write, ssize_t(const void *buf, size_t count));

	ssize_t p_read(void *buf, size_t count) {
		int ret = count < remain ? count : remain;
		memcpy(buf, data + returned, ret);
		remain -= ret;
		returned += ret;
		return ret;
	};
	void set_transmitdata(void *buf, size_t count) {
		if (data)
			free(data);
		data = (unsigned char *)malloc(count);
		memcpy(data, buf, count);
		remain = count, returned = 0;
	}

  protected:
	int returned;
	size_t remain;
	unsigned char *data;
};

TEST(mock_MeterOMS, basic_nokey) {
	mock_OMShwif *hwif = new mock_OMShwif();
	std::list<Option> opt;

	ASSERT_THROW(MeterOMS m(opt, hwif), vz::VZException);
}

TEST(mock_MeterOMS, basic) {
	mock_OMShwif *hwif = new mock_OMShwif();
	std::list<Option> opt;
	opt.push_back(Option("key", "0078580E79544B145D1A96D0F7E777FA"));

	MeterOMS m(opt, hwif);
	ASSERT_EQ(SUCCESS, m.open()); // if scanW1devices returns false open has to return ERR
	m.close();                    // this might be called and should not cause problems
}

// first sync: 1040F03016h -> E5h?
TEST(mock_MeterOMS, sync) {
	mock_OMShwif *hwif = new mock_OMShwif();
	std::list<Option> opt;
	opt.push_back(Option("key", "0078580E79544B145D1A96D0F7E777FA"));

	unsigned char data[5] = {0x10, 0x40, 0xf0, 0x30, 0x16};
	hwif->set_transmitdata(data, 5);

	EXPECT_CALL(*hwif, read(_, _))
		.Times(AtLeast(1))
		.WillRepeatedly(Invoke(hwif, &mock_MeterOMS::mock_OMShwif::p_read));
	EXPECT_CALL(*hwif, write(_, _)).Times(1).WillRepeatedly(Return(1)); // assume only E5h
	MeterOMS m(opt, hwif);
	ASSERT_EQ(SUCCESS, m.open()); // if scanW1devices returns false open has to return ERR
	std::vector<Reading> rds(1);
	ASSERT_EQ(m.read(rds, 1), 0);
	m.close(); // this might be called and should not cause problems
}

TEST(mock_MeterOMS, first_packets) {
	mock_OMShwif *hwif = new mock_OMShwif();
	std::list<Option> opt;
	opt.push_back(Option("key", "0078580E79544B145D1A96D0F7E777FA"));

	// 1040F03016
	// 685F5F6873F05B000000002D4C010E0000500581A000A0D241D0E1A8B6F4F8D03C212E6099FA3B4A36D81B8D01863F583881093354F7ADD2EC10123CBBFE868C14FFF087594691B4D795C12E853401B2DC085CFB1AEED000A19E9DCEA65015392D153B9816685F5F6853F05B000000002D4C010E01005005FBF582B297275D1A6A208BB161FDB4F17EECCA54DD3A1D42FBFEF4B5F50EF90B96FDB5FCF00784F8DCD1A6B77D17427AB2C985E2738B6B4E603D5730F34C5BC40208972B994C9D29D878A62C18717E182916
	// 685F5F6873F05B000000002D4C010E020050057DF252604C329DAAACB814F7FC8E2DC8B102ACF9A2005FB5570782B36500120272C5B34811628C2D24DC8733B805373AEE51E919FB42E50AE4E9C1F5861F94BA9A5654239E7E781513BC831C1456DE900A16685F5F6853F05B000000002D4C010E030050054538B4887F7EE4BCC67F83A3042126ED3BC11B58A6B6292F192DC6806F817CD043CE51750B809903EC405E7508D238AA5374447F7C07AADE1F4046A64BB8CD5E1F69480C6E6216D4D340E4E32AEB24724416685F5F6873F05B000000002D4C010E04005005B0C26D2A2650AD30FB982C58121877A718FEFD15AAC1DABC92F5255253B36FC18699703DA742F743E

	// unsigned char data[200] = {0x10, 0x40, 0xf0, 0x30, 0x16, 0x68, 0x5F, 0x5F, 0x68, 0x53, 0xF0,
	// 0x5B, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x4C, 0x01, 0x0E, 0x0D, 0x00, 0x50, 0x05, 0x3F, 0xD0,
	// 0xFE, 0xB7, 0x26, 0x76, 0x0C, 0xC7, 0xAA, 0xF0, 0xB5, 0x2B, 0x41, 0xF0, 0xC5, 0x41, 0xBD,
	// 0x63, 0x06, 0xDC, 0xD8, 0xB9, 0x1B, 0x3D, 0xA2, 0x31, 0x1E, 0xF1, 0x3D, 0x25,
	// 0x14, 0xD0, 0x96, 0x00, 0x82, 0x16, 0x1E, 0xFE, 0xC4, 0xB6, 0xCB, 0x1E, 0x0B,
	// 0x33, 0x28, 0xBE, 0x61, 0x77, 0xDC, 0xA5, 0x94, 0xC1, 0x28, 0x00, 0x24, 0xA8,
	// 0x35, 0xF1, 0xD6, 0x55, 0xBA, 0x71, 0x82, 0xB2, 0x56, 0xE9, 0x4B, 0xD3, 0x3A,
	// 0xC0, 0xA6, 0xB0, 0x8D, 0xA4, 0x67, 0x81, 0xEB, 0x4E, 0x91, 0xE0, 0x12, 0x16 };
	unsigned char data[300] = {
		0x10, 0x40, 0xF0, 0x30, 0x16, 0x68, 0x5F, 0x5F, 0x68, 0x73, 0xF0, 0x5B, 0x00, 0x00, 0x00,
		0x00, 0x2D, 0x4C, 0x01, 0x0E, 0x00, 0x00, 0x50, 0x05, 0x81, 0xA0, 0x00, 0xA0, 0xD2, 0x41,
		0xD0, 0xE1, 0xA8, 0xB6, 0xF4, 0xF8, 0xD0, 0x3C, 0x21, 0x2E, 0x60, 0x99, 0xFA, 0x3B, 0x4A,
		0x36, 0xD8, 0x1B, 0x8D, 0x01, 0x86, 0x3F, 0x58, 0x38, 0x81, 0x09, 0x33, 0x54, 0xF7, 0xAD,
		0xD2, 0xEC, 0x10, 0x12, 0x3C, 0xBB, 0xFE, 0x86, 0x8C, 0x14, 0xFF, 0xF0, 0x87, 0x59, 0x46,
		0x91, 0xB4, 0xD7, 0x95, 0xC1, 0x2E, 0x85, 0x34, 0x01, 0xB2, 0xDC, 0x08, 0x5C, 0xFB, 0x1A,
		0xEE, 0xD0, 0x00, 0xA1, 0x9E, 0x9D, 0xCE, 0xA6, 0x50, 0x15, 0x39, 0x2D, 0x15, 0x3B, 0x98,
		0x16, 0x68, 0x5F, 0x5F, 0x68, 0x53, 0xF0, 0x5B, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x4C, 0x01,
		0x0E, 0x01, 0x00, 0x50, 0x05, 0xFB, 0xF5, 0x82, 0xB2, 0x97, 0x27, 0x5D, 0x1A, 0x6A, 0x20,
		0x8B, 0xB1, 0x61, 0xFD, 0xB4, 0xF1, 0x7E, 0xEC, 0xCA, 0x54, 0xDD, 0x3A, 0x1D, 0x42, 0xFB,
		0xFE, 0xF4, 0xB5, 0xF5, 0x0E, 0xF9, 0x0B, 0x96, 0xFD, 0xB5, 0xFC, 0xF0, 0x07, 0x84, 0xF8,
		0xDC, 0xD1, 0xA6, 0xB7, 0x7D, 0x17, 0x42, 0x7A, 0xB2, 0xC9, 0x85, 0xE2, 0x73, 0x8B, 0x6B,
		0x4E, 0x60, 0x3D, 0x57, 0x30, 0xF3, 0x4C, 0x5B, 0xC4, 0x02, 0x08, 0x97, 0x2B, 0x99, 0x4C,
		0x9D, 0x29, 0xD8, 0x78, 0xA6, 0x2C, 0x18, 0x71, 0x7E, 0x18, 0x29, 0x16};
	hwif->set_transmitdata(data, 300);

	EXPECT_CALL(*hwif, read(_, _))
		.Times(AtLeast(1))
		.WillRepeatedly(Invoke(hwif, &mock_MeterOMS::mock_OMShwif::p_read));
	EXPECT_CALL(*hwif, write(_, _)).Times(3).WillRepeatedly(Return(1)); // assume only E5h
	MeterOMS m(opt, hwif);
	ASSERT_EQ(SUCCESS, m.open()); // if scanW1devices returns false open has to return ERR
	std::vector<Reading> rds(10);
	ASSERT_EQ(m.read(rds, 10), 8);
	m.close(); // this might be called and should not cause problems
	// check whether one reading has the proper time:
	struct tm t;
	t.tm_sec = 3;
	t.tm_min = 36;
	t.tm_hour = 23;
	t.tm_mday = 20;
	t.tm_mon = 6 - 1;
	t.tm_year = 2015 - 1900;
	t.tm_isdst = -1;
	ASSERT_EQ(rds[7].time_s(), mktime(&t));
}

} // namespace mock_MeterOMS

void print(log_level_t l, char const *s1, char const *s2, ...) {
	//	if (l!= log_debug)
	{
		fprintf(stdout, "\n  %s:", s2);
		va_list argp;
		va_start(argp, s2);
		vfprintf(stdout, s1, argp);
		va_end(argp);
		fprintf(stdout, "\n");
	}
}

int main(int argc, char **argv) {
	// The following line must be executed to initialize Google Mock
	// (and Google Test) before running the tests.
	::testing::InitGoogleMock(&argc, argv);
	return RUN_ALL_TESTS();
}
