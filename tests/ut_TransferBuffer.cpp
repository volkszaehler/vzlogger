/*
 * unit tests for TransferBuffer.cpp
 */

#include "gtest/gtest.h"

#include <Buffer.hpp>
#include <TransferBuffer.hpp>
#include <vector>

template <class IT> inline void copyto(Buffer &buf, IT first, IT last) {
	for (; first != last; ++first)
		buf.push(*first);
}

TEST(TransferBuffer, insert) {
	vz::TransferBuffer buf;
	Buffer source;
	ReadingIdentifier::Ptr pRid;

	std::vector<Reading> data{
		{1.0, {1, 0}, pRid},
		{2.0, {2, 0}, pRid},
		{2.1, {3, 0}, pRid},
		{2.2, {40, 1}, pRid},
	};

	copyto(source, data.begin(), data.end());
	auto n = buf.append(source, "TBufTest");
	ASSERT_EQ(n, 4);
	ASSERT_EQ(buf.size(), 4);
	ASSERT_EQ(buf.begin()->time_ms(), 1000);
	ASSERT_EQ((--buf.end())->time_ms(), 40000);
	for (const auto &r : source)
		ASSERT_TRUE(r.deleted());

	for (const auto &r : buf)
		ASSERT_FALSE(r.deleted());

	n = buf.discard(); // keeps last element of history
	ASSERT_TRUE(buf.empty());
	ASSERT_EQ(buf.size(), 0);
	ASSERT_EQ(n, 4);

	// all elements of source are marked deleted -> copy none
	n = buf.append(source, "TBufTest");
	ASSERT_EQ(n, 0);
	ASSERT_EQ(buf.size(), 0);
}

TEST(TransferBuffer, filter_by_time) {
	ReadingIdentifier::Ptr pRid;
	vz::TransferBuffer buf;
	Buffer source;

	std::vector<Reading> data{{1.0, {1, 0}, pRid},   {2.0, {2, 0}, pRid},   {2.1, {3, 0}, pRid},
							  {2.1, {3, 200}, pRid}, {2.1, {3, 499}, pRid}, {2.1, {3, 500}, pRid},
							  {2.1, {3, 990}, pRid}, {2.1, {3, 1000}, pRid}};

	copyto(source, data.begin(), data.end());
	auto n = buf.append(source, "TBufTest");
	EXPECT_EQ(n, 4);
	EXPECT_EQ(buf.size(), 4);
	EXPECT_EQ(buf.begin()->time_ms(), 1000);
	EXPECT_EQ((--buf.end())->time_ms(), 3001);

	buf.discard(); // keep last element of history
	source.undelete();
	buf.append(source, "TBufTest"); // skips elements with dt <= 0
	EXPECT_TRUE(buf.empty());
}

TEST(TransferBuffer, filter_duplicates) {
	ReadingIdentifier::Ptr pRid;
	vz::TransferBuffer buf;
	Buffer source;

	std::vector<Reading> data{{1.0, {1, 0}, pRid},   {2.0, {2, 0}, pRid},      {2.0, {3, 0}, pRid},
							  {2.0, {4, 200}, pRid}, {2.0, {4, 999999}, pRid}, {2.0, {5, 0}, pRid},
							  {2.1, {5, 1000}, pRid}};

	copyto(source, data.begin(), data.end());
	auto n = buf.append(source, "TBufTest");
	EXPECT_EQ(n, 7);
	EXPECT_EQ(buf.size(), 7);

	buf.clear(); // keep last element of history
	source.undelete();
	n = buf.append(source, "TBufTest", 3000); // skips duplicates with dt < 3 s
	EXPECT_EQ(n, 4);
	auto it = buf.begin();
	ASSERT_EQ(it++->time_ms(), 1000);
	ASSERT_EQ(it++->time_ms(), 2000);
	ASSERT_EQ(it++->time_ms(), 5000);
	ASSERT_EQ(it++->time_ms(), 5001);
}
