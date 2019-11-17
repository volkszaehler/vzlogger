#include "gmock/gmock.h"
#include "gtest/gtest.h"

class MockTurtle {
  public:
	MOCK_METHOD0(PenUp, void());
	MOCK_METHOD0(PenDown, void());
	MOCK_METHOD1(Forward, void(int distance));
	MOCK_CONST_METHOD0(GetY, int());
};

using ::testing::AtLeast;

TEST(SampleTest, AssertionTrue) {
	MockTurtle turtle;
	EXPECT_CALL(turtle, PenDown()).Times(AtLeast(1));

	turtle.PenDown();
	ASSERT_EQ(1, 1);
}
