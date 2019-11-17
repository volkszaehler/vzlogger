#include <gmock/gmock.h>
using ::testing::_;
using ::testing::AtLeast;
using ::testing::Eq;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::ReturnRef;

#include <gtest/gtest.h>
using ::testing::Test;

#include "Meter.hpp"
#include "protocols/MeterW1therm.hpp"

namespace mock_MeterW1therm {

class mock_W1hwif : public MeterW1therm::W1HWif {
  public:
	mock_W1hwif(){};
	virtual ~mock_W1hwif(){};
	MOCK_METHOD0(scanW1devices, bool());
	MOCK_CONST_METHOD0(W1devices, const std::list<std::string> &());
	MOCK_METHOD2(readTemp, bool(const std::string &dev, double &value));
};

TEST(mock_MeterW1therm, basic_fail_scanW1devices) {
	mock_W1hwif *hwif = new mock_W1hwif();
	std::list<Option> opt;

	EXPECT_CALL(*hwif, scanW1devices()).Times(1).WillRepeatedly(Return(false));
	EXPECT_CALL(*hwif, W1devices()).Times(0);
	MeterW1therm m(opt, hwif);
	ASSERT_EQ(ERR, m.open()); // if scanW1devices returns false open has to return ERR
	m.close();                // this might be called and should not cause problems
}

TEST(mock_MeterW1therm, basic_scanW1devices_empty) {
	mock_W1hwif *hwif = new mock_W1hwif();
	std::list<Option> opt;

	EXPECT_CALL(*hwif, scanW1devices()).Times(1).WillRepeatedly(Return(true));
	std::list<std::string> devs;
	EXPECT_CALL(*hwif, W1devices()).Times(AtLeast(1)).WillRepeatedly(ReturnRef(devs));
	MeterW1therm m(opt, hwif);
	ASSERT_EQ(SUCCESS, m.open());
	EXPECT_CALL(*hwif, readTemp(_, _)).Times(0);
	std::vector<Reading> rds(1);
	EXPECT_TRUE(0 == m.read(rds, 1));
	ASSERT_EQ(SUCCESS, m.close());
}

TEST(mock_MeterW1therm, basic_scanW1devices_1nok) {
	mock_W1hwif *hwif = new mock_W1hwif();
	std::list<Option> opt;

	EXPECT_CALL(*hwif, scanW1devices()).Times(1).WillRepeatedly(Return(true));
	std::list<std::string> devs;
	devs.push_back(std::string("dev1"));
	EXPECT_CALL(*hwif, W1devices()).Times(AtLeast(1)).WillRepeatedly(ReturnRef(devs));
	MeterW1therm m(opt, hwif);
	ASSERT_EQ(SUCCESS, m.open());
	EXPECT_CALL(*hwif, readTemp(_, _)).Times(1).WillRepeatedly(Return(false));
	std::vector<Reading> rds(1);
	EXPECT_TRUE(0 == m.read(rds, 1));

	ASSERT_EQ(SUCCESS, m.close());
}

TEST(mock_MeterW1therm, basic_scanW1devices_2nok) {
	mock_W1hwif *hwif = new mock_W1hwif();
	std::list<Option> opt;

	EXPECT_CALL(*hwif, scanW1devices()).Times(1).WillRepeatedly(Return(true));
	std::list<std::string> devs;
	devs.push_back(std::string("dev1"));
	devs.push_back(std::string("dev2"));
	EXPECT_CALL(*hwif, W1devices()).Times(AtLeast(1)).WillRepeatedly(ReturnRef(devs));
	MeterW1therm m(opt, hwif);
	ASSERT_EQ(SUCCESS, m.open());
	EXPECT_CALL(*hwif, readTemp(_, _)).Times(2).WillRepeatedly(Return(false));
	std::vector<Reading> rds(1);
	EXPECT_TRUE(0 == m.read(rds, 1));

	ASSERT_EQ(SUCCESS, m.close());
}

TEST(mock_MeterW1therm, basic_scanW1devices_1ok) {
	mock_W1hwif *hwif = new mock_W1hwif();
	std::list<Option> opt;

	EXPECT_CALL(*hwif, scanW1devices()).Times(1).WillRepeatedly(Return(true));
	std::list<std::string> devs;
	devs.push_back(std::string("dev1"));
	EXPECT_CALL(*hwif, W1devices()).Times(AtLeast(1)).WillRepeatedly(ReturnRef(devs));
	MeterW1therm m(opt, hwif);
	ASSERT_EQ(SUCCESS, m.open());
	EXPECT_CALL(*hwif, readTemp(_, _)).Times(1).WillRepeatedly(Return(true));
	std::vector<Reading> rds(1);
	EXPECT_TRUE(1 == m.read(rds, 1));

	ASSERT_EQ(SUCCESS, m.close());
}

TEST(mock_MeterW1therm, basic_scanW1devices_2ok_n1) {
	mock_W1hwif *hwif = new mock_W1hwif();
	std::list<Option> opt;

	EXPECT_CALL(*hwif, scanW1devices()).Times(1).WillRepeatedly(Return(true));
	std::list<std::string> devs;
	devs.push_back(std::string("dev1"));
	devs.push_back(std::string("dev2"));
	EXPECT_CALL(*hwif, W1devices()).Times(AtLeast(1)).WillRepeatedly(ReturnRef(devs));
	MeterW1therm m(opt, hwif);
	ASSERT_EQ(SUCCESS, m.open());
	EXPECT_CALL(*hwif, readTemp(_, _)).Times(1).WillRepeatedly(Return(true));
	std::vector<Reading> rds(1);
	EXPECT_TRUE(1 == m.read(rds, 1));

	ASSERT_EQ(SUCCESS, m.close());
}

TEST(mock_MeterW1therm, basic_scanW1devices_2ok) {
	mock_W1hwif *hwif = new mock_W1hwif();
	std::list<Option> opt;

	EXPECT_CALL(*hwif, scanW1devices()).Times(1).WillRepeatedly(Return(true));
	std::list<std::string> devs;
	devs.push_back(std::string("dev1"));
	devs.push_back(std::string("dev2"));
	EXPECT_CALL(*hwif, W1devices()).Times(AtLeast(1)).WillRepeatedly(ReturnRef(devs));
	MeterW1therm m(opt, hwif);
	ASSERT_EQ(SUCCESS, m.open());
	EXPECT_CALL(*hwif, readTemp("dev1", _)).Times(1).WillOnce(Return(true));
	EXPECT_CALL(*hwif, readTemp("dev2", _)).Times(1).WillOnce(Return(true));
	std::vector<Reading> rds(2);
	EXPECT_TRUE(2 == m.read(rds, 2));

	ASSERT_EQ(SUCCESS, m.close());
}

} // namespace mock_MeterW1therm

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
