#include "gtest/gtest.h"

#include "CurlSessionProvider.hpp"

TEST(CurlSessionProvider, init) {
	ASSERT_EQ(nullptr, curlSessionProvider);
	curlSessionProvider = new CurlSessionProvider();

	delete curlSessionProvider;
	curlSessionProvider = nullptr;
}

TEST(CurlSessionProvider, single1) {
	curlSessionProvider = new CurlSessionProvider();

	ASSERT_FALSE(curlSessionProvider->inUse("1"));

	CURL *eh = curlSessionProvider->get_easy_session("1");
	ASSERT_TRUE(nullptr != eh);
	ASSERT_TRUE(curlSessionProvider->inUse("1"));
	curlSessionProvider->return_session("1", eh);
	ASSERT_EQ(nullptr, eh);
	ASSERT_FALSE(curlSessionProvider->inUse("1"));

	// 2nd try:
	eh = curlSessionProvider->get_easy_session("1");
	ASSERT_TRUE(nullptr != eh);
	ASSERT_TRUE(curlSessionProvider->inUse("1"));
	curlSessionProvider->return_session("1", eh);
	ASSERT_EQ(nullptr, eh);
	ASSERT_FALSE(curlSessionProvider->inUse("1"));

	delete curlSessionProvider;
	curlSessionProvider = nullptr;

	// TODO create that that's spanws a thread and tests blocking on a shared session
}
