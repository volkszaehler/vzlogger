/*
 * unit tests for MeterOCR.cpp
 * Author: Matthias Behr, 2015
 */

#include <json-c/json.h>
#include <protocols/MeterOCR.hpp>

#include <test_config.hpp>

#include "gtest/gtest.h"

class MeterOCR_Test {
  public:
	static void test_calcImpulses();
	static void test_roundBasedOnSmallerDigits();
};

TEST(MeterOCR, basic2_needle_autofix) {
	std::list<Option> options;
	options.emplace_back("file", ocrTestImage("img2.png"));
	options.push_back(
		Option("rotate", -2.0)); // rotate by -2deg (counterclockwise)// 661, 539, 585/680
	struct json_object *jso = json_tokener_parse("[{\"type\": \"needle\", \"boundingboxes\":[\
	{\"identifier\": \"water cons\", \"scaler\":-1,\"digit\":false, \"circle\": {\"cx\": 689, \"cy\": 449, \"cr\": 24}},\
	{\"identifier\": \"water cons\", \"scaler\":-2,\"digit\":false, \"circle\": {\"cx\": 661, \"cy\": 539, \"cr\": 24}},\
	{\"identifier\": \"water cons\", \"scaler\":-3,\"digit\":false, \"circle\": {\"cx\": 574, \"cy\": 576, \"cr\": 24}, \"confidence_id\": \"conf_circle\"},\
	{\"identifier\": \"water cons\", \"scaler\":-4,\"digit\":true, \"circle\": {\"cx\": 488, \"cy\": 542, \"cr\": 24}}\
	]}]");                       // should detect 0,3767
	options.push_back(Option("recognizer", jso));
	json_object_put(jso);
	jso = json_tokener_parse("\
	{\"range\": 20, \"x\": 465, \"y\":395}\
	");
	options.push_back(Option("autofix", jso));
	json_object_put(jso);

	MeterOCR m(options);

	ASSERT_EQ(SUCCESS, m.open());

	std::vector<Reading> rds;
	rds.resize(2);
	EXPECT_EQ(2, m.read(rds, 2));

	double value = rds[0].value();
	EXPECT_TRUE(std::abs(0.3767 - value) < 0.00001);

	value = rds[1].value();
	EXPECT_GT(value, 80.0); // expect conf. >80
	ASSERT_EQ(0, m.close());
}

TEST(MeterOCR, basic2_needle_autofix_impulses) {
	std::list<Option> options;
	options.emplace_back("file", ocrTestImage("img2_blue.png"));
	options.push_back(Option("generate_debug_image", false));
	options.push_back(Option("impulses", 10000));
	options.push_back(
		Option("rotate", -2.0)); // rotate by -2deg (counterclockwise)// 661, 539, 585/680
	struct json_object *jso =
		json_tokener_parse("[{\"type\": \"needle\",\"kernelColorString\":\"-2 -2 2 0 0 0 0 0 0\",\
  \"boundingboxes\":[\
  {\"identifier\": \"water cons\", \"scaler\":-1,\"digit\":false, \"circle\": {\"cx\": 689, \"cy\": 449, \"cr\": 24}},\
  {\"identifier\": \"water cons\", \"scaler\":-2,\"digit\":false, \"circle\": {\"cx\": 661, \"cy\": 539, \"cr\": 24}},\
  {\"identifier\": \"water cons\", \"scaler\":-3,\"digit\":false, \"circle\": {\"cx\": 574, \"cy\": 576, \"cr\": 24}, \"confidence_id\": \"conf_circle\"},\
  {\"identifier\": \"water cons\", \"scaler\":-4,\"digit\":true, \"circle\": {\"cx\": 488, \"cy\": 542, \"cr\": 24}}\
  ],\
  }]"); // should detect 0,3767
	options.push_back(Option("recognizer", jso));
	json_object_put(jso);
	jso = json_tokener_parse("\
  {\"range\": 20, \"x\": 465, \"y\":395}\
  ");
	options.push_back(Option("autofix", jso));
	json_object_put(jso);

	MeterOCR m(options);

	ASSERT_EQ(SUCCESS, m.open());

	std::vector<Reading> rds;
	rds.resize(2);
	EXPECT_EQ(0, m.read(rds, 2));
	m.set_forced_file_changed();
	// expect 0 readings on first read

	ASSERT_EQ(2, m.read(rds, 2));
	// expect 2 on 2nd read

	double value = rds[0].value(); // but with same pic no impulses. // TODO add interface to change
								   // file between reads
	EXPECT_TRUE(std::abs(value) < 0.00001);

	value = rds[1].value();
	EXPECT_GT(value, 80.0); // expect conf. >80

	ASSERT_EQ(0, m.close());
}

TEST(MeterOCR, basic2_needle_autofix_impulses_debug_image) {
	std::list<Option> options;
	options.emplace_back("file", ocrTestImage("img2.png"));
	options.push_back(Option("generate_debug_image", true));
	options.push_back(Option("impulses", 10000));
	options.push_back(
		Option("rotate", -2.0)); // rotate by -2deg (counterclockwise)// 661, 539, 585/680
	struct json_object *jso = json_tokener_parse("[{\"type\": \"needle\", \"boundingboxes\":[\
	{\"identifier\": \"water cons\", \"scaler\":-1,\"digit\":false, \"circle\": {\"cx\": 689, \"cy\": 449, \"cr\": 24}},\
	{\"identifier\": \"water cons\", \"scaler\":-2,\"digit\":false, \"circle\": {\"cx\": 661, \"cy\": 539, \"cr\": 24}},\
	{\"identifier\": \"water cons\", \"scaler\":-3,\"digit\":false, \"circle\": {\"cx\": 574, \"cy\": 576, \"cr\": 24}, \"confidence_id\": \"conf_circle\"},\
	{\"identifier\": \"water cons\", \"scaler\":-4,\"digit\":true, \"circle\": {\"cx\": 488, \"cy\": 542, \"cr\": 24}}\
	]}]");                       // should detect 0,3767
	options.push_back(Option("recognizer", jso));
	json_object_put(jso);
	jso = json_tokener_parse("\
	{\"range\": 20, \"x\": 465, \"y\":395}\
	");
	options.push_back(Option("autofix", jso));
	json_object_put(jso);

	MeterOCR m(options);

	ASSERT_EQ(SUCCESS, m.open());

	std::vector<Reading> rds;
	rds.resize(2);
	EXPECT_EQ(0, m.read(rds, 2));
	m.set_forced_file_changed();
	// expect 0 readings on first read

	EXPECT_EQ(2, m.read(rds, 2));
	// expect 2 on 2nd read

	double value = rds[0].value(); // but with same pic no impulses. // TODO add interface to change
								   // file between reads
	EXPECT_TRUE(std::abs(value) < 0.00001);

	value = rds[1].value();
	EXPECT_GT(value, 80.0); // expect conf. >80

	ASSERT_EQ(0, m.close());
}

void MeterOCR_Test::test_calcImpulses() {
	std::list<Option> options;
	options.emplace_back("file", ocrTestImage("img2.png"));
	options.push_back(Option("impulses", 10));
	struct json_object *jso = json_tokener_parse("[{\"type\": \"needle\", \"boundingboxes\":[\
	{\"identifier\": \"water cons\", \"scaler\":-1,\"digit\":false, \"circle\": {\"cx\": 689, \"cy\": 449, \"cr\": 24}},\
	{\"identifier\": \"water cons\", \"scaler\":-2,\"digit\":false, \"circle\": {\"cx\": 661, \"cy\": 539, \"cr\": 24}},\
	{\"identifier\": \"water cons\", \"scaler\":-3,\"digit\":false, \"circle\": {\"cx\": 574, \"cy\": 576, \"cr\": 24}, \"confidence_id\": \"conf_circle\"},\
	{\"identifier\": \"water cons\", \"scaler\":-4,\"digit\":true, \"circle\": {\"cx\": 488, \"cy\": 542, \"cr\": 24}}\
	]}]"); // should detect 0,3767
	options.push_back(Option("recognizer", jso));

	MeterOCR m(options);
	// let's test calcImpulses:
	ASSERT_EQ(0, m.calcImpulses(0.1, 0.1));
	ASSERT_EQ(0, m.calcImpulses(-0.1, -0.1));
	ASSERT_EQ(0, m.calcImpulses(1001.2, 1001.2));

	// max impulses:
	ASSERT_EQ(5, m.calcImpulses(5.5, 5.0));
	ASSERT_EQ(-5, m.calcImpulses(5.0, 5.49999));

	// wrap detection:
	ASSERT_EQ(4, m.calcImpulses(
					 0.2, 0.8)); // without wrap det. it would be negative (new value < old value)
	// multiple wraps:
	ASSERT_EQ(4, m.calcImpulses(0.2, 10.8));
	ASSERT_EQ(-4, m.calcImpulses(10.8, 0.2));

	// now with impulses <10 (i.e. no wrap detection)
	m._impulses = 5;
	ASSERT_EQ(5, m.calcImpulses(6.0, 5.0));
	ASSERT_EQ(-5, m.calcImpulses(5.0, 6.0));
	ASSERT_EQ(-3, m.calcImpulses(0.2, 0.8)); // with wrap it's +5
}

TEST(MeterOCR, calcImpulses) { MeterOCR_Test::test_calcImpulses(); }

void MeterOCR_Test::test_roundBasedOnSmallerDigits() {
	struct json_object *jso = json_tokener_parse("\
		  {\"boundingboxes\": [{\"identifier\":\"1\", \"circle\":{\"cx\":20, \"cy\":20, \"cr\":10}}]\
		  }");
	MeterOCR::RecognizerNeedle r(jso);
	json_object_put(jso);
	int conf = 100;
	// no rounding
	ASSERT_EQ(1, r.roundBasedOnSmallerDigits(1, 1.0, 0.0, conf));
	EXPECT_EQ(100, conf);
	// round up:
	ASSERT_EQ(5, r.roundBasedOnSmallerDigits(4, 4.99, 0.0, conf));
	EXPECT_EQ(90, conf);
	// round down:
	ASSERT_EQ(4, r.roundBasedOnSmallerDigits(5, 5.00, 0.99, conf));
	EXPECT_EQ(75, conf);

	// round down:
	conf += 15;
	ASSERT_EQ(4, r.roundBasedOnSmallerDigits(5, 5.4, 0.91, conf));
	EXPECT_EQ(75, conf);

	// round up wrap:
	ASSERT_EQ(0, r.roundBasedOnSmallerDigits(9, 9.99, 0.0, conf));
	EXPECT_EQ(65, conf);

	// round down wrap:
	ASSERT_EQ(9, r.roundBasedOnSmallerDigits(0, 0.00, 0.99, conf));
	EXPECT_EQ(50, conf);
	conf += 15;
	ASSERT_EQ(9, r.roundBasedOnSmallerDigits(0, 0.4, 0.91, conf));
	EXPECT_EQ(50, conf);

	// stay:
	ASSERT_EQ(4, r.roundBasedOnSmallerDigits(4, 4.5, 0.5, conf));
	EXPECT_EQ(50, conf);
	ASSERT_EQ(0, r.roundBasedOnSmallerDigits(0, 0.4, 0.90, conf));
	EXPECT_EQ(50, conf);

	// stay:
	ASSERT_EQ(4, r.roundBasedOnSmallerDigits(4, 4.5, 0.1, conf));
	EXPECT_EQ(50, conf);
	// stay:
	ASSERT_EQ(4, r.roundBasedOnSmallerDigits(4, 4.5, 0.9, conf));
	EXPECT_EQ(50, conf);

	// stay:
	ASSERT_EQ(4, r.roundBasedOnSmallerDigits(4, 4.9, 0.9, conf));
	EXPECT_EQ(50, conf);
}

TEST(MeterOCR, roundBasedOnSmallerDigits) { MeterOCR_Test::test_roundBasedOnSmallerDigits(); }

TEST(MeterOCR, debouncing) {
	ASSERT_EQ(8, debounce(9, 8.49));
	ASSERT_EQ(9, debounce(9, 8.51));
	ASSERT_EQ(9, debounce(9, 8.99));
	ASSERT_EQ(9, debounce(9, 9.00));
	ASSERT_EQ(9, debounce(9, 9.49));
	ASSERT_EQ(9, debounce(9, 9.50));
	ASSERT_EQ(9, debounce(9, 9.51));
	ASSERT_EQ(9, debounce(9, 9.99));
	//	ASSERT_EQ(9, debounce(9, 10.0)); // should never happen
	ASSERT_EQ(9, debounce(9, 0.49));
	ASSERT_EQ(0, debounce(9, 0.51));
	ASSERT_EQ(9, debounce(0, 9.49));
	ASSERT_EQ(0, debounce(0, 9.51));

	for (int i = 0; i < 9; ++i) {
		ASSERT_EQ(i, debounce(i, (double)i + 0.5));
		ASSERT_EQ(i, debounce(i, (double)i + 1.0));
		ASSERT_EQ(i + 1, debounce(i, (double)i + 1.501));
		ASSERT_EQ(i + 1, debounce(i + 1, (double)(i + 1) - 0.4999));
		ASSERT_EQ(i, debounce(i + 1, (double)(i + 1) - 0.51));
	}
};

TEST(MeterOCR,
	 basic2_needle_v4l) // TODO remove this test as it depends on a test machine with v4l2 dev
{
	std::list<Option> options;
	options.push_back(Option("v4l2_dev", (char *)"/dev/video0"));
	options.push_back(
		Option("rotate", -2.0)); // rotate by -2deg (counterclockwise)// 661, 539, 585/680
	struct json_object *jso = json_tokener_parse("[{\"type\": \"needle\", \"boundingboxes\":[\
		{\"identifier\": \"water cons\", \"scaler\":-1,\"digit\":false, \"circle\": {\"cx\": 689, \"cy\": 449, \"cr\": 24}},\
		{\"identifier\": \"water cons\", \"scaler\":-2,\"digit\":false, \"circle\": {\"cx\": 661, \"cy\": 539, \"cr\": 24}},\
		{\"identifier\": \"water cons\", \"scaler\":-3,\"digit\":false, \"circle\": {\"cx\": 574, \"cy\": 576, \"cr\": 24}, \"confidence_id\": \"conf_circle\"},\
		{\"identifier\": \"water cons\", \"scaler\":-4,\"digit\":true, \"circle\": {\"cx\": 488, \"cy\": 542, \"cr\": 24}}\
		 ]}]");                  // should detect 0,3767
	options.push_back(Option("recognizer", jso));
	json_object_put(jso);
	jso = json_tokener_parse("\
		{\"range\": 20, \"x\": 465, \"y\":395}\
	  ");
	options.push_back(Option("autofix", jso));
	json_object_put(jso);

	MeterOCR m(options);

	if (SUCCESS ==
		m.open()) { // we don't use assert_eq here as not all build machines have a camera!

		//	ASSERT_EQ(SUCCESS, m.open());

		std::vector<Reading> rds;
		rds.resize(2);
		EXPECT_EQ(0, m.read(rds, 2));
		EXPECT_EQ(0, m.read(rds, 2));
		EXPECT_EQ(0, m.read(rds, 2));
		EXPECT_EQ(0, m.read(rds, 2));

		/*
  double value = rds[0].value();
  EXPECT_TRUE(std::abs(0.3767-value)<0.00001);

  value = rds[1].value();
  EXPECT_GT(value, 80.0); // expect conf. >80
		*/
		ASSERT_EQ(0, m.close());
	}
}
