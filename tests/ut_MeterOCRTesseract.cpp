/*
 * unit tests for MeterOCR.cpp
 * Author: Matthias Behr, 2015
 */

#include "protocols/MeterOCR.hpp"
#include "gtest/gtest.h"
#include "json/json.h"

// this is a dirty hack. we should think about better ways/rules to link against the
// test objects.
// e.g. single files in the tests directory that just include the real file. This way one
// will avoid multiple includes of the same file in a different test case
// (or linking against the real file from the CMakeLists.txt)

#include "../src/protocols/MeterOCRTesseract.cpp"

TEST(MeterOCRTesseract, basic_prepared) {
	std::list<Option> options;
	options.push_back(Option("file", (char *)"tests/meterOCR/sensus_test1_nronly2.png"));
	ASSERT_THROW(MeterOCR m2(options), vz::VZException); // missing boundingboxes parameter
	struct json_object *jsa =
		json_tokener_parse("[{\"boundingboxes\":[{\"identifier\": \"id1\"}]}]");
	ASSERT_TRUE(jsa != 0);
	options.push_back(Option("recognizer", jsa));
	json_object_put(jsa);
	MeterOCR m(options);

	ASSERT_EQ(SUCCESS, m.open());

	std::vector<Reading> rds;
	rds.resize(1);
	EXPECT_EQ(1, m.read(rds, 1));

	double value = rds[0].value();
	EXPECT_EQ(432, value);

	ASSERT_EQ(0, m.close());
}

TEST(MeterOCRTesseract, basic_scaler) {
	std::list<Option> options;
	options.push_back(Option("file", (char *)"tests/meterOCR/sensus_test1_nronly2.png"));
	ASSERT_THROW(MeterOCR m2(options), vz::VZException); // missing boundingboxes parameter
	struct json_object *jsa = json_tokener_parse("[{\"boundingboxes\":[{\"identifier\": \"id1\", "
												 "\"scaler\": -2, \"confidence_id\":\"cid1\"}]}]");
	ASSERT_TRUE(jsa != 0);
	options.push_back(Option("recognizer", jsa));
	json_object_put(jsa);
	MeterOCR m(options);

	ASSERT_EQ(SUCCESS, m.open());

	std::vector<Reading> rds;
	rds.resize(2);
	EXPECT_EQ(2, m.read(rds, 2));

	double value = rds[0].value();
	EXPECT_TRUE(abs(4.32 - value) < 0.001);

	ReadingIdentifier *p = rds[1].identifier().get();
	StringIdentifier *o = dynamic_cast<StringIdentifier *>(p);
	StringIdentifier s1("cid1");
	EXPECT_TRUE(o->operator==(s1));
	value = rds[1].value();
	EXPECT_GT(value, 80.0); // expect conf. >80

	ASSERT_EQ(0, m.close());
}

TEST(MeterOCRTesseract, basic_two_boxes_same_id) {
	std::list<Option> options;
	options.push_back(Option("file", (char *)"tests/meterOCR/sensus_test1_nronly2.png"));
	ASSERT_THROW(MeterOCR m2(options), vz::VZException); // missing boundingboxes parameter
	struct json_object *jsa = json_tokener_parse(
		"[{\"boundingboxes\":[{\"identifier\": \"id1\"},{\"identifier\": \"id1\"}]}]");
	ASSERT_TRUE(jsa != 0);
	options.push_back(Option("recognizer", jsa));
	json_object_put(jsa);
	MeterOCR m(options);

	ASSERT_EQ(SUCCESS, m.open());

	std::vector<Reading> rds;
	rds.resize(1);
	EXPECT_EQ(1, m.read(rds, 1));

	double value = rds[0].value();
	EXPECT_EQ(2.0 * 432, value);

	ASSERT_EQ(0, m.close());
}
/* the better we make the image preparation the more some edges get detected as digits
TEST(MeterOCRTesseract, basic_not_prepared)
{
	std::list<Option> options;
	options.push_back(Option("file", (char*)"tests/meterOCR/sensus_test1_nronly1.png"));
	options.push_back(Option("rotate", -2.0)); // rotate by -2deg (counterclockwise)
	struct json_object *jso = json_tokener_parse("[{\"identifier\": \"water cons\", \"box\":
{\"x1\": 42, \"x2\": 395, \"y1\": 10, \"y2\": 65}}]"); // should detect 00432m
	options.push_back(Option("boundingboxes", jso));
	json_object_put(jso);
	MeterOCR m(options);

	ASSERT_EQ(SUCCESS, m.open());

	std::vector<Reading> rds;
	rds.resize(1);
	EXPECT_EQ(1, m.read(rds, 1));

	double value = rds[0].value();
	EXPECT_EQ(432, value);

	ASSERT_EQ(0, m.close());
}
*/
TEST(MeterOCRTesseract, basic_not_prepared_digits) {
	std::list<Option> options;
	options.push_back(Option("file", (char *)"tests/meterOCR/sensus_test1_nronly1.png"));
	options.push_back(Option("rotate", -2.0)); // rotate by -2deg (counterclockwise)
	struct json_object *jso = json_tokener_parse("[{\"boundingboxes\":[\
	{\"identifier\": \"water cons\", \"scaler\":4,\"digit\":true, \"box\": {\"x1\": 41, \"x2\": 75, \"y1\": 20, \"y2\": 66}},\
	{\"identifier\": \"water cons\", \"scaler\":3,\"digit\":true, \"box\": {\"x1\": 104, \"x2\": 136, \"y1\": 20, \"y2\": 66}},\
	{\"identifier\": \"water cons\", \"scaler\":2,\"digit\":true, \"box\": {\"x1\": 166, \"x2\": 195, \"y1\": 20, \"y2\": 66}},\
	{\"identifier\": \"water cons\", \"scaler\":1,\"digit\":true, \"box\": {\"x1\": 229, \"x2\": 264, \"y1\": 20, \"y2\": 66}},\
	{\"identifier\": \"water cons\", \"scaler\":0,\"digit\":true, \"box\": {\"x1\": 290, \"x2\": 325, \"y1\": 20, \"y2\": 66}, \"confidence_id\":\"confidence\"}\
	]}]");                                     // should detect 00432
	options.push_back(Option("recognizer", jso));
	json_object_put(jso);
	MeterOCR m(options);

	ASSERT_EQ(SUCCESS, m.open());

	std::vector<Reading> rds;
	rds.resize(2);
	EXPECT_EQ(2, m.read(rds, 2));

	double value = rds[0].value();
	EXPECT_EQ(432, value);

	value = rds[1].value();
	EXPECT_GT(value, 80.0); // expect conf. >80

	ASSERT_EQ(0, m.close());
}

TEST(MeterOCRTesseract, DISABLED_emh_test2_not_prepared) // TODO enable once lcd digits are working
{
	std::list<Option> options;
	options.push_back(Option("file", (char *)"tests/meterOCR/emh_test2.png"));
	options.push_back(Option("gamma", 0.8));
	options.push_back(Option("gamma_min", 50));
	options.push_back(Option("gamma_max", 180));
	// options.push_back(Option("rotate", -2.0)); // rotate by -2deg (counterclockwise)
	struct json_object *jso = json_tokener_parse("[{\"boundingboxes\":[\
	{\"identifier\": \"cons HT\", \"box\": {\"x1\": 122, \"x2\": 235, \"y1\": 8, \"y2\": 54}},\
	{\"identifier\": \"cons NT\", \"box\": {\"x1\": 122 , \"x2\": 235, \"y1\": 58, \"y2\": 104}}]}]");
	options.push_back(Option("recognizer", jso));
	json_object_put(jso);
	MeterOCR m(options);

	ASSERT_EQ(SUCCESS, m.open());

	std::vector<Reading> rds;
	rds.resize(2);
	EXPECT_EQ(2, m.read(rds, 2));

	double value = rds[0].value();
	EXPECT_EQ(757.6, value); // 2nd value 11557.0
	value = rds[1].value();
	EXPECT_EQ(557.0,
			  value); // 2nd value 11557.0 but we moved the boundingbox due to some light effect.

	ASSERT_EQ(0, m.close());
}

TEST(MeterOCRTesseract,
	 DISABLED_emh_test2_not_prepared_digits) // TODO enable once lcd digits are working
{
	std::list<Option> options;
	options.push_back(Option("file", (char *)"tests/meterOCR/emh_test2.png"));
	options.push_back(Option("gamma", 0.8));
	options.push_back(Option("gamma_min", 50));
	options.push_back(Option("gamma_max", 180));
	// options.push_back(Option("rotate", -2.0)); // rotate by -2deg (counterclockwise)
	struct json_object *jso = json_tokener_parse("[{\"boundingboxes\":[\
	{\"identifier\": \"cons HT\", \"digit\":true, \"scaler\":2,\"box\": {\"x1\": 122, \"x2\": 151, \"y1\": 8, \"y2\": 54}},\
	{\"identifier\": \"cons HT\", \"digit\":true, \"scaler\":1,\"box\": {\"x1\": 152, \"x2\": 180, \"y1\": 8, \"y2\": 54}},\
	{\"identifier\": \"cons HT\", \"digit\":true, \"scaler\":0,\"box\": {\"x1\": 181, \"x2\": 208, \"y1\": 8, \"y2\": 54}},\
	{\"identifier\": \"cons HT\", \"digit\":true, \"scaler\":-1,\"box\": {\"x1\": 212, \"x2\": 235, \"y1\": 8, \"y2\": 54}},\
	{\"identifier\": \"cons NT\", \"digit\":true, \"scaler\":2,\"box\": {\"x1\": 122, \"x2\": 151, \"y1\": 58, \"y2\": 104}},\
	{\"identifier\": \"cons NT\", \"digit\":true, \"scaler\":1,\"box\": {\"x1\": 152, \"x2\": 180, \"y1\": 58, \"y2\": 104}},\
	{\"identifier\": \"cons NT\", \"digit\":true, \"scaler\":0,\"box\": {\"x1\": 181, \"x2\": 208, \"y1\": 58, \"y2\": 104}},\
	{\"identifier\": \"cons NT\", \"digit\":true, \"scaler\":-1,\"box\": {\"x1\": 212, \"x2\": 235, \"y1\": 58, \"y2\":104}}\
	]}]");
	options.push_back(Option("recognizer", jso));
	json_object_put(jso);
	MeterOCR m(options);

	ASSERT_EQ(SUCCESS, m.open());

	std::vector<Reading> rds;
	rds.resize(2);
	EXPECT_EQ(2, m.read(rds, 2));

	double value = rds[0].value();
	EXPECT_EQ(757.6, value); // 2nd value 11557.0
	value = rds[1].value();
	EXPECT_EQ(557.0,
			  value); // 2nd value 11557.0 but we moved the boundingbox due to some light effect.

	ASSERT_EQ(0, m.close());
}

TEST(MeterOCRTesseract, basic2_not_prepared_digits) {
	std::list<Option> options;
	options.push_back(Option("file", (char *)"tests/meterOCR/img.png"));
	options.push_back(Option("rotate", -2.0)); // rotate by -2deg (counterclockwise)
	struct json_object *jso = json_tokener_parse("[{\"boundingboxes\":[\
	{\"identifier\": \"water cons\", \"scaler\":4,\"digit\":true, \"box\": {\"x1\": 465, \"x2\": 487, \"y1\": 358, \"y2\": 395}},\
	{\"identifier\": \"water cons\", \"scaler\":3,\"digit\":true, \"box\": {\"x1\": 502, \"x2\": 525, \"y1\": 358, \"y2\": 395}},\
	{\"identifier\": \"water cons\", \"scaler\":2,\"digit\":true, \"box\": {\"x1\": 538, \"x2\": 562, \"y1\": 358, \"y2\": 395}},\
	{\"identifier\": \"water cons\", \"scaler\":1,\"digit\":true, \"box\": {\"x1\": 575, \"x2\": 599, \"y1\": 358, \"y2\": 395}},\
	{\"identifier\": \"water cons\", \"scaler\":0,\"digit\":true, \"box\": {\"x1\": 610, \"x2\": 637, \"y1\": 358, \"y2\": 395}}\
	]}]");                                     // should detect 00434
	options.push_back(Option("recognizer", jso));
	json_object_put(jso);

	MeterOCR m(options);

	ASSERT_EQ(SUCCESS, m.open());

	for (int i = 0; i < 4; ++i) {
		std::vector<Reading> rds;
		rds.resize(1);
		EXPECT_EQ(1, m.read(rds, 1));

		double value = rds[0].value();
		EXPECT_EQ(434, value);
		m.set_forced_file_changed(); // otherwise next read call will assume image is unchanged
	}
	ASSERT_EQ(0, m.close());
}

TEST(MeterOCRTesseract, basic2_not_prepared_autofix) {
	std::list<Option> options;
	options.push_back(Option("file", (char *)"tests/meterOCR/img2.png"));
	options.push_back(Option("rotate", -2.0)); // rotate by -2deg (counterclockwise)
	struct json_object *jso = json_tokener_parse("[{\"boundingboxes\":[\
	{\"identifier\": \"water cons\", \"scaler\":4,\"digit\":true, \"box\": {\"x1\": 465, \"x2\": 487, \"y1\": 358, \"y2\": 395}},\
	{\"identifier\": \"water cons\", \"scaler\":3,\"digit\":true, \"box\": {\"x1\": 502, \"x2\": 525, \"y1\": 358, \"y2\": 395}},\
	{\"identifier\": \"water cons\", \"scaler\":2,\"digit\":true, \"box\": {\"x1\": 538, \"x2\": 562, \"y1\": 358, \"y2\": 395}},\
	{\"identifier\": \"water cons\", \"scaler\":1,\"digit\":true, \"box\": {\"x1\": 575, \"x2\": 599, \"y1\": 358, \"y2\": 395}},\
	{\"identifier\": \"water cons\", \"scaler\":0,\"digit\":true, \"box\": {\"x1\": 610, \"x2\": 637, \"y1\": 358, \"y2\": 395}, \"confidence_id\": \"conf\"}\
	]}]");                                     // should detect 00434
	options.push_back(Option("recognizer", jso));
	json_object_put(jso);
	jso = json_tokener_parse("\
	{\"range\": 20, \"x\": 465, \"y\":395}\
	");
	options.push_back(Option("autofix", jso));
	json_object_put(jso);

	MeterOCR m(options);

	ASSERT_EQ(SUCCESS, m.open());

	for (int i = 0; i < 1; ++i) {
		std::vector<Reading> rds;
		rds.resize(2);
		EXPECT_EQ(2, m.read(rds, 2));

		double value = rds[0].value();
		EXPECT_EQ(435, value);

		value = rds[1].value();
		EXPECT_GT(value, 80.0); // expect conf. >80
	}
	ASSERT_EQ(0, m.close());
}
