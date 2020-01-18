/*
 * unit tests for Meter.cpp
 * Author: Matthias Behr, 2014
 */

#include "gtest/gtest.h"

#include <Buffer.hpp>

TEST(buffer, buffer_agg_avg) {
	Buffer buf;
	buf.set_aggmode(Buffer::AVG);

	ReadingIdentifier::Ptr pRid;
	struct timeval t1;
	t1.tv_sec = 1;
	t1.tv_usec = 0;
	{
		Reading r1(1.0, t1, pRid);
		buf.push(r1);

		buf.aggregate(0, false);
		// now assert exact one, not deleted:
		ASSERT_EQ(buf.size(), (size_t)1);
		Reading &r = *buf.begin();
		ASSERT_TRUE(!r.deleted());
		// first case: no prev. value, just one data -> return value as AVG.
		ASSERT_EQ(r.value(), 1.0);
		r.mark_delete();
	}
	// now add a 2nd value:
	{
		t1.tv_sec = 2;
		Reading r2(2.0, t1, pRid);
		buf.push(r2);
		buf.aggregate(0, false);
		buf.clean();
		ASSERT_EQ(buf.size(), (size_t)1);
		Reading &r = *buf.begin();
		ASSERT_TRUE(!r.deleted());
		// 2nd case: prev. value (1.0 at 1s), just one new data (2.0 at 2s)-> return 1.0 as AVG (2.0
		// has no time yet!)
		ASSERT_EQ(r.value(), 1.0);
		r.mark_delete();
	}
	// now add 2 values:
	{
		t1.tv_sec = 4;
		Reading r3(3.0, t1, pRid);
		buf.push(r3);
		t1.tv_sec = 7;
		Reading r4(4.0, t1, pRid);
		buf.push(r4);

		buf.aggregate(0, false);
		buf.clean();
		ASSERT_EQ(buf.size(), (size_t)1);
		Reading &r = *buf.begin();
		ASSERT_TRUE(!r.deleted());
		// 3rd case: prev. value (2.0 at 2s), two new data (3.0 at 4s and 4.0 at 7s)-> return
		// (2*2+3*3)/5 as AVG (4.0 has no time yet!)
		ASSERT_EQ(((2.0 * 2.0) + (3.0 * 3.0)) / 5.0, r.value());
		r.mark_delete();
	}
}

TEST(buffer, clean) {
	// call clean on empty buffer:
	Buffer buf;
	buf.clean();

	Buffer buf2;
	buf2.clean(false);

	// add one item:
	ReadingIdentifier::Ptr pRid;
	struct timeval t1;
	t1.tv_sec = 1;
	t1.tv_usec = 0;
	Reading r1(1.0, t1, pRid);

	buf.push(r1);
	ASSERT_EQ(1ul, buf.size());

	// call default clean() that does only remove deleted items:
	buf.clean();
	ASSERT_EQ(1ul, buf.size());

	(*buf.begin()).mark_delete();
	// call default clean() that does only remove deleted items:
	buf.clean();
	ASSERT_EQ(0ul, buf.size());

	Reading r2(2.0, t1, pRid);
	buf.push(r2);
	buf.clean(false);
	ASSERT_EQ(0ul, buf.size());
}
