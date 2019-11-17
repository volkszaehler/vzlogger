#include "PushData.hpp"
#include "gtest/gtest.h"

// dirty hack until we find a better solution:
#include "../src/PushData.cpp"

TEST(PushData, PDL_basic_constructor) {
	ASSERT_EQ(0, pushDataList);
	PushDataList pdl;
	ASSERT_EQ(0, pushDataList);
}

TEST(PushData, PDL_basic_add) {
	PushDataList pdl;
	pdl.add("0", 1, 1.0);
	// let pdl destroy it
}

TEST(PushData, PDL_basic_waitForData) {
	PushDataList pdl;
	pdl.add("0", 1, 1.0);
	PushDataList::DataMap *dm = pdl.waitForData();
	ASSERT_TRUE(0 != dm);
	ASSERT_EQ(1ul, dm->size());
	delete dm;
}

TEST(PushData, PDL_basic_waitForData2) {
	PushDataList pdl;
	pdl.add("0", 1, 1.0);
	pdl.add("0", 2, 2.0);
	PushDataList::DataMap *dm = pdl.waitForData();
	ASSERT_TRUE(0 != dm);
	ASSERT_EQ(1ul, dm->size()); // still one uuid
	ASSERT_EQ(2ul, dm->operator[]("0").size());
	delete dm;
}

TEST(PushData, PDL_basic_waitForData3) {
	PushDataList pdl;
	pdl.add("0", 1, 1.0);
	pdl.add("0", 2, 2.0);
	pdl.add("1", 3, 3.0);
	PushDataList::DataMap *dm = pdl.waitForData();
	ASSERT_TRUE(0 != dm);
	ASSERT_EQ(2ul, dm->size()); // now two uuids
	ASSERT_EQ(2ul, dm->operator[]("0").size());
	ASSERT_EQ(1ul, dm->operator[]("1").size());
	delete dm;
}

TEST(PushData, PDL_basic_waitForData4) {
	PushDataList pdl;
	pdl.add("0", 1, 1.0);
	PushDataList::DataMap *dm = pdl.waitForData();
	ASSERT_TRUE(0 != dm);
	ASSERT_EQ(1ul, dm->size());
	delete dm;
	pdl.add("1", 4, 4.4);
	dm = pdl.waitForData();
	ASSERT_TRUE(0 != dm);
	ASSERT_EQ(1ul, dm->size());
	ASSERT_EQ(4.4, dm->operator[]("1").back().second);
	delete dm;
}

// todo if we'd provide a timeout to waitForData we could test here the case with empty data

class PushDataServerTest {
  public:
	PushDataServerTest(PushDataServer &pds) : _pds(pds){};
	std::string generateJson(PushDataList::DataMap &dataMap) { return _pds.generateJson(dataMap); }
	size_t size() { return _pds._middlewareList.size(); };
	PushDataServer &_pds;
};

TEST(PushData, PDS_only_constructor) { PushDataServer pds(0); }

TEST(PushData, PDS_constructor) {
	PushDataServer pds(0);
	PushDataServerTest pt(pds);

	PushDataList pdl;
	pdl.add("0", 1, 1.1);
	PushDataList::DataMap *dm = pdl.waitForData();
	ASSERT_TRUE(0 != dm);
	std::string str = pt.generateJson(*dm);
	// todo fix the comparision! 1.10000 vs. 1.10000001, ... ASSERT_EQ("{ \"data\": [ { \"uuid\":
	// \"0\", \"tuples\": [ [ 1, 1.100000 ] ] } ] }", str);
}

TEST(PushData, PDS_fail_middleware) {
	ASSERT_EQ(0, curlSessionProvider);
	curlSessionProvider = new CurlSessionProvider();

	struct json_object *jso =
		json_tokener_parse("[{\"url\": \"http://127.0.0.1:45431/unit_test/push.json\"}]");
	PushDataServer pds(jso);
	json_object_put(jso);
	PushDataServerTest pt(pds);
	ASSERT_EQ(1ul, pt.size());
	PushDataList pdl;
	pdl.add("0", 1, 1.1);
	pushDataList = &pdl;
	ASSERT_FALSE(pds.waitAndSendOnceToAll()); // we assume that localhost:45431/unit_test/push.json
											  // can't be connected to
	pushDataList = 0;

	delete curlSessionProvider;
	curlSessionProvider = 0;
}
