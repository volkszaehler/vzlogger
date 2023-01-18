#include <gmock/gmock.h>
using ::testing::_;
using ::testing::AtLeast;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArrayArgument;

#include <gtest/gtest.h>
using ::testing::Test;

#include "Channel.hpp"
#include "Config_Options.hpp"
#include "Meter.hpp"
#include "MeterMap.hpp"

namespace mock_metermap {

class mock_meter : public Meter {
  public:
	mock_meter(const std::list<Option> &o) : Meter(o){};
	virtual ~mock_meter(){};
	//    mock_meter() : Meter(std::list<Option>()) {};
	MOCK_METHOD0(open, void());
	MOCK_METHOD0(close, int());
	MOCK_CONST_METHOD0(isEnabled, bool());
	MOCK_METHOD2(read, size_t(std::vector<Reading> &rds, size_t n));
};

// here we test class MeterMap and threads.cpp:
// we mock the other classes used

TEST(mock_metermap, basic_open_close_enabled) {
	std::list<Option> o;
	o.push_back(Option("protocol", "random"));
	mock_meter *mtr = new mock_meter(o);
	mtr->interval(1);
	testing::Mock::AllowLeak(mtr);
	EXPECT_CALL(*mtr, isEnabled()).Times(AtLeast(1)).WillRepeatedly(Return(true));
	{
		InSequence d;
		EXPECT_CALL(*mtr, open()).Times(1);
		EXPECT_CALL(*mtr, close()); // Times(1) assumed
	}

	MeterMap m(mtr);
	m.start();
	m.cancel();
	EXPECT_FALSE(m.running());
}

// test that disabled meters don't get open/closed

TEST(mock_metermap, basic_open_close_not_enabled) {
	std::list<Option> o;
	o.push_back(Option("protocol", "random"));
	mock_meter *mtr = new mock_meter(o);
	mtr->interval(1);
	EXPECT_CALL(*mtr, isEnabled()).Times(AtLeast(1));
	EXPECT_CALL(*mtr, open()).Times(0);
	EXPECT_CALL(*mtr, close()).Times(0);

	MeterMap m(mtr);
	m.start();
	m.cancel();
	EXPECT_FALSE(m.running());
}

TEST(mock_metermap, DISABLED_one_channel) { // todo issue #400
	std::list<Option> o;
	o.push_back(Option("protocol", "random"));
	mock_meter *mtr = new mock_meter(o);
	mtr->interval(1);
	EXPECT_CALL(*mtr, isEnabled()).Times(AtLeast(1)).WillRepeatedly(Return(true));
	{
		InSequence d;
		EXPECT_CALL(*mtr, open()).Times(1);
		EXPECT_CALL(*mtr, close()); // Times(1) assumed
	}

	MeterMap m(mtr);
	Channel *ch = new Channel();
	EXPECT_CALL(*ch, name()).Times(AtLeast(1));
	EXPECT_CALL(*ch, identifier())
		.Times(AtLeast(1))
		.WillRepeatedly(Return(ReadingIdentifier::Ptr()));
	EXPECT_CALL(*ch, buffer()).Times(AtLeast(1)).WillRepeatedly(Invoke(ch, &Channel::real_buf));
	EXPECT_CALL(*ch, notify()).Times(AtLeast(0)); // can be called 0 or sometimes
	{
		InSequence s;
		EXPECT_CALL(*ch, start(_)).Times(1);
		EXPECT_CALL(*ch, cancel()).Times(1);
		EXPECT_CALL(*ch, join()).Times(1);
	}
	// TODO Bug? Channel::Notify get's called even without a single reading gathered!
	m.push_back(Channel::Ptr(ch));
	EXPECT_CALL(*mtr, read(_, Ge(1u))).Times(AtLeast(1)).WillRepeatedly(Return(1));

	m.start();
	usleep(100000); // give reading thread a chance. todo better cancel that synchronizes with
					// readingthread
	m.cancel();
	EXPECT_FALSE(m.running());
}

// test whether the read data get's only into the proper channels: (two channel can have same id)
size_t return_read(std::vector<Reading> &rds, size_t n) {
	if (n > 0)
		rds[0] = Reading(ReadingIdentifier::Ptr(new ChannelIdentifier(1)));
	return 1;
}

TEST(mock_metermap, DISABLED_reading_in_proper_channel) { // todo issue #400
	std::list<Option> o;
	o.push_back((Option("protocol", "random")));
	mock_meter *mtr = new mock_meter(o);
	EXPECT_CALL(*mtr, isEnabled()).Times(AtLeast(1)).WillRepeatedly(Return(true));

	MeterMap m(mtr);
	mtr->interval(1);
	Channel *ch1 = new Channel();
	Channel *ch2 = new Channel();
	Channel *ch3 = new Channel();

	// assign ids: 1 2 1 (so ch1.id == ch3.id)
	EXPECT_CALL(*ch1, identifier())
		.Times(AtLeast(1))
		.WillRepeatedly(Return(ReadingIdentifier::Ptr(new ChannelIdentifier(1))));
	EXPECT_CALL(*ch2, identifier())
		.Times(AtLeast(1))
		.WillRepeatedly(Return(ReadingIdentifier::Ptr(new ChannelIdentifier(2))));
	EXPECT_CALL(*ch3, identifier())
		.Times(AtLeast(1))
		.WillRepeatedly(Return(ReadingIdentifier::Ptr(new ChannelIdentifier(1))));

	// make sure the channels have a real buffer:
	EXPECT_CALL(*ch1, buffer()).Times(AtLeast(1)).WillRepeatedly(Invoke(ch1, &Channel::real_buf));
	EXPECT_CALL(*ch2, buffer()).Times(AtLeast(1)).WillRepeatedly(Invoke(ch2, &Channel::real_buf));
	EXPECT_CALL(*ch3, buffer()).Times(AtLeast(1)).WillRepeatedly(Invoke(ch3, &Channel::real_buf));

	// our test. Make sure that ch1 and ch3 get one reading and ch2 gets none
	EXPECT_CALL(*ch1, push(_)).Times(1);
	EXPECT_CALL(*ch2, push(_)).Times(0);
	EXPECT_CALL(*ch3, push(_)).Times(1);

	EXPECT_CALL(*mtr, read(_, Ge(1u))).Times(AtLeast(1)).WillRepeatedly(Invoke(return_read));

	m.push_back(Channel::Ptr(ch1));
	m.push_back(Channel::Ptr(ch2));
	m.push_back(Channel::Ptr(ch3));
	m.start();
	usleep(100000);
	m.cancel();
}

} // namespace mock_metermap

Config_Options options;

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
