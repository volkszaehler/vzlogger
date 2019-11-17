#include "gtest/gtest.h"

#include "CurlSessionProvider.hpp"

TEST(CurlSessionProvider, init) {
	ASSERT_EQ(0, curlSessionProvider);
	curlSessionProvider = new CurlSessionProvider();

	delete curlSessionProvider;
	curlSessionProvider = 0;
}

TEST(CurlSessionProvider, single1) {
	curlSessionProvider = new CurlSessionProvider();

	ASSERT_FALSE(curlSessionProvider->inUse("1"));

	CURL *eh = curlSessionProvider->get_easy_session("1");
	ASSERT_TRUE(0 != eh);
	ASSERT_TRUE(curlSessionProvider->inUse("1"));
	curlSessionProvider->return_session("1", eh);
	ASSERT_EQ(0, eh);
	ASSERT_FALSE(curlSessionProvider->inUse("1"));

	// 2nd try:
	eh = curlSessionProvider->get_easy_session("1");
	ASSERT_TRUE(0 != eh);
	ASSERT_TRUE(curlSessionProvider->inUse("1"));
	curlSessionProvider->return_session("1", eh);
	ASSERT_EQ(0, eh);
	ASSERT_FALSE(curlSessionProvider->inUse("1"));

	delete curlSessionProvider;
	curlSessionProvider = 0;

	// TODO create that that's spanws a thread and tests blocking on a shared session
}
