#include <regex>

#include <Buffer.hpp>
#include <Channel.hpp>
#include <Config_Options.hpp>
#include <api/Volkszaehler.hpp>
// #include <api/CurlResponse.hpp>

#include "gtest/gtest.h"
#include <test_config.hpp>

// another dirty hack to test private functions from Volkszaehler:
namespace vz {
namespace api {
class Volkszaehler_Test {
  public:
	static void api_parse_exception(vz::api::Volkszaehler &v, vz::api::CURLresponse &r, char *&err,
									size_t &n) {
		v.api_parse_exception(r, err, n);
	}
	static std::list<Reading> &values(Volkszaehler &v) { return v._values; }
	static json_object *api_json_tuples(Volkszaehler &v, Buffer::Ptr buf) {
		return v.api_json_tuples(buf);
	};
};
} // namespace api
} // namespace vz

Config_Options options(config_file());

#ifdef HAVE_CPP_REGEX

TEST(api_Volkszaehler, regex_for_configs) {
	// ^\s*(#|$)
	std::regex regex("^\\s*(//(.*|)|$)");
	EXPECT_TRUE(std::regex_match("", regex));
	EXPECT_TRUE(std::regex_match("//", regex));
	EXPECT_TRUE(std::regex_match("//bla", regex));
	EXPECT_TRUE(std::regex_match("// bla", regex));
	EXPECT_TRUE(std::regex_match("\t//", regex));
	EXPECT_TRUE(std::regex_match("\t// bla", regex));
	EXPECT_TRUE(std::regex_match("  //", regex));
	// some neg. tests:
	EXPECT_FALSE(std::regex_match("bla", regex));
}

#endif

TEST(api_Volkszaehler, config_options) {
	MapContainer mappings;
#ifdef OCR_SUPPORT
	options.config_parse(mappings); // let's see whether we can parse the provided example config
#else
	// doesn't throw anylonger since OCR removed by basic example conf.
	// ASSERT_THROW(options.config_parse(mappings), vz::VZException); // the default config should
	// fail due to missing Protocol ocr. There might be another failure... so parse it anyhow here:
	options.config_parse(mappings);
#endif
}

TEST(api_Volkszaehler, no_middleware) {
	using namespace vz::api;
	std::list<Option> options;
	// options.push_front(Option("middleware", (char*)"bla_middleware"));
	ReadingIdentifier::Ptr pRid;
	Channel *ch = new Channel(options, std::string("bla_api"), std::string("bla_uuid"), pRid);
	Channel::Ptr chp(ch);
	ASSERT_THROW(Volkszaehler v(chp, options), vz::VZException);
}

TEST(api_Volkszaehler, api_parse_exception) {
	using namespace vz::api;
	CURLresponse resp;
	resp.data = 0;
	resp.size = 0;
	char *err = new char[100];
	err[0] = 0;
	size_t n = 100;
	std::list<Option> options;
	options.push_front(Option("middleware", (char *)"bla_middleware"));
	ReadingIdentifier::Ptr pRid;
	Channel *ch = new Channel(options, std::string("bla_api"), std::string("bla_uuid"), pRid);
	Channel::Ptr chp(ch);
	Volkszaehler v(chp, options);

	// test using empty data:
	// todo bug: this writes in err even if its null!
	Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	ASSERT_STREQ("continue", err) << err;

	// test using small size for n:
	n = 5;
	Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	// todo bug: doesn't zero terminate the string! ASSERT_STREQ("conti", err ) << err;
	n = 100;

	// test using corrupted resp data:
	resp.size = 100;
	resp.data = 0;
	// todo: this crashes! Volkszaehler_Test::api_parse_exception(v, resp, err, n);

	// test using empty json resp:
	resp.data = (char *)"{}%";
	resp.size = strlen(resp.data);
	Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	ASSERT_STREQ("Missing exception", err);

	// test using just exception:
	resp.data = (char *)"{exception}%";
	resp.size = strlen(resp.data);
	Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	ASSERT_STREQ("quoted object property name expected", err);

	// test using just "exception":
	resp.data = (char *)"{\"exception\"}%";
	resp.size = strlen(resp.data);
	Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	ASSERT_STREQ("object property name separator ':' expected", err);

	// test using proper "exception":
	resp.data = (char *)"{\"exception\":\"1\"}%";
	resp.size = strlen(resp.data);
	// todo bug throws a c++ exception (null not valid) Volkszaehler_Test::api_parse_exception(v,
	// resp, err, n); ASSERT_STREQ("object property name separator ':' expected", err);

	// test using proper "exception" but not type/message:
	resp.data = (char *)"{\"exception\": { \"type\":\"1\", \"message\":\"2\"  } }%";
	resp.size = strlen(resp.data);
	Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	ASSERT_STREQ("'1': '2'", err);

	// test type: "UniqueConstraintViolationException" message contains "Duplicate entry"
	resp.data = (char *)"{\"exception\": { \"type\":\"UniqueConstraintViolationException\", "
						"\"message\":\"2 Duplicate entry\"  } }%";
	resp.size = strlen(resp.data);
	// todo bug: crashes with double free if _values is empty!
	// Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	// ASSERT_STREQ("'UniqueConstraintViolationException': '2 Duplicate entry'", err);
	Volkszaehler_Test::values(v).push_front(Reading());
	Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	ASSERT_TRUE(0 == Volkszaehler_Test::values(v).size());
	ASSERT_STREQ("'UniqueConstraintViolationException': '2 Duplicate entry'", err);

	delete[] err;
}

TEST(api_Volkszaehler, api_json_tuples_no_duplicates) {
	using namespace vz::api;
	std::list<Option> options;
	options.push_front(Option("middleware", (char *)"bla_middleware"));
	ReadingIdentifier::Ptr pRid;
	Channel *ch = new Channel(options, std::string("bla_api"), std::string("bla_uuid"), pRid);
	Channel::Ptr chp(ch);
	Volkszaehler v(chp, options);

	// test using empty data:
	json_object *j = Volkszaehler_Test::api_json_tuples(v, ch->buffer());
	ASSERT_TRUE(j == 0);
	ASSERT_TRUE(Volkszaehler_Test::values(v).size() == 0);

	struct timeval t1;
	t1.tv_sec = 1;
	t1.tv_usec = 1;
	struct timeval t2;
	t2.tv_sec = 2;
	t2.tv_usec = 1;
	Reading r1(1.0, t1, pRid);
	Reading r2(1.0, t2, pRid);
	ch->push(r1);

	// expect one data returned in values:
	j = Volkszaehler_Test::api_json_tuples(v, ch->buffer());
	ASSERT_TRUE(j != 0);
	ASSERT_TRUE(Volkszaehler_Test::values(v).size() == 1);
	ASSERT_EQ(Volkszaehler_Test::values(v).front(), r1);
	json_object_put(j);
	ch->buffer()->clean(); // remove deleted
	ASSERT_TRUE(ch->buffer()->size() == 0);

	ch->push(r1);
	ch->push(r2);
	// expect only two data returned in values: (r1 ignored as timestamp same as previous) and r2)
	j = Volkszaehler_Test::api_json_tuples(v, ch->buffer());
	ASSERT_TRUE(j != 0);
	ASSERT_TRUE(Volkszaehler_Test::values(v).size() == 2);
	ASSERT_EQ(Volkszaehler_Test::values(v).front(), r1);
	ASSERT_EQ(Volkszaehler_Test::values(v).back(), r2);

	json_object_put(j);
	ch->buffer()->clean(); // remove deleted
	ASSERT_TRUE(ch->buffer()->size() == 0);
}

TEST(api_Volkszaehler, api_json_tuples_duplicates) {
	using namespace vz::api;
	std::list<Option> options;
	options.push_front(Option("middleware", (char *)"bla_middleware"));
	options.push_back(Option("duplicates", 10));
	ReadingIdentifier::Ptr pRid;
	Channel *ch = new Channel(options, std::string("bla_api"), std::string("bla_uuid"), pRid);
	Channel::Ptr chp(ch);
	Volkszaehler v(chp, options);

	// test using empty data:
	json_object *j = Volkszaehler_Test::api_json_tuples(v, ch->buffer());
	ASSERT_TRUE(j == 0);
	ASSERT_TRUE(Volkszaehler_Test::values(v).size() == 0);

	struct timeval t1;
	t1.tv_sec = 1;
	t1.tv_usec = 1;
	struct timeval t2;
	t2.tv_sec = 2;
	t2.tv_usec = 1;
	Reading r1(1.0, t1, pRid);
	Reading r2(1.0, t2, pRid);
	ch->push(r1);

	// expect one data returned in values:
	j = Volkszaehler_Test::api_json_tuples(v, ch->buffer());
	ASSERT_TRUE(j != 0);
	ASSERT_TRUE(Volkszaehler_Test::values(v).size() == 1);
	ASSERT_EQ(Volkszaehler_Test::values(v).front(), r1);
	json_object_put(j);
	ch->buffer()->clean(); // remove deleted
	ASSERT_TRUE(ch->buffer()->size() == 0);

	ch->push(r1);
	ch->push(r2);
	// expect one data returned in values: (r1 same timestamp, r2 ignored)
	j = Volkszaehler_Test::api_json_tuples(v, ch->buffer());
	ASSERT_TRUE(j != 0);
	ASSERT_TRUE(Volkszaehler_Test::values(v).size() == 1);
	ASSERT_EQ(Volkszaehler_Test::values(v).front(), r1);

	json_object_put(j);
	ch->buffer()->clean(); // remove deleted
	ASSERT_TRUE(ch->buffer()->size() == 0);

	struct timeval t3;
	t3.tv_sec = 2;
	t3.tv_usec = 1 + 1000; // at least one ms distance
	Reading r3(2.0, t3, pRid);
	ch->push(r3);

	// now add one with a different value:
	// then we should get r1 and the new value r3:
	j = Volkszaehler_Test::api_json_tuples(v, ch->buffer());
	ASSERT_TRUE(j != 0);
	ASSERT_TRUE(Volkszaehler_Test::values(v).size() == 2);
	ASSERT_EQ(Volkszaehler_Test::values(v).front(), r1);
	Volkszaehler_Test::values(v).pop_front();
	ASSERT_EQ(Volkszaehler_Test::values(v).front(), r3);

	json_object_put(j);
	ASSERT_TRUE(ch->buffer()->size() == 0);

	// now try timeout:
	// add r4 with same value but distance >10s
	t1.tv_sec = t3.tv_sec + 10;
	t1.tv_usec = t3.tv_usec;
	Reading r4(2.0, t1, pRid);
	ch->push(r4);
	// now there should be r3 and r4:
	j = Volkszaehler_Test::api_json_tuples(v, ch->buffer());
	ASSERT_TRUE(j != 0);
	ASSERT_TRUE(Volkszaehler_Test::values(v).size() == 2) << Volkszaehler_Test::values(v).size();
	ASSERT_EQ(Volkszaehler_Test::values(v).front(), r3);
	Volkszaehler_Test::values(v).pop_front();
	ASSERT_EQ(Volkszaehler_Test::values(v).front(), r4);
	Volkszaehler_Test::values(v).pop_front();

	json_object_put(j);
	ASSERT_TRUE(ch->buffer()->size() == 0);

	// now try timeout and value change:
	// expect only new value to be added:
	t1.tv_sec += 11;
	Reading r5(5.0, t1, pRid);
	ch->push(r5);
	j = Volkszaehler_Test::api_json_tuples(v, ch->buffer());
	ASSERT_TRUE(j != 0);
	ASSERT_TRUE(Volkszaehler_Test::values(v).size() == 1) << Volkszaehler_Test::values(v).size();
	ASSERT_EQ(Volkszaehler_Test::values(v).front(), r5);
	Volkszaehler_Test::values(v).pop_front();

	json_object_put(j);
	ASSERT_TRUE(ch->buffer()->size() == 0);

	// now ignore one
	t1.tv_sec += 1;
	Reading r6(5.0, t1, pRid);
	ch->push(r6);
	j = Volkszaehler_Test::api_json_tuples(v, ch->buffer());
	ASSERT_TRUE(j == 0);
	ASSERT_TRUE(Volkszaehler_Test::values(v).size() == 0) << Volkszaehler_Test::values(v).size();

	ASSERT_TRUE(ch->buffer()->size() == 0);

	// and try timeout and value change again
	// expect new value to be added:

	t1.tv_sec += 10;
	Reading r7(7.0, t1, pRid);
	ch->push(r7);
	j = Volkszaehler_Test::api_json_tuples(v, ch->buffer());
	ASSERT_TRUE(j != 0);
	ASSERT_TRUE(Volkszaehler_Test::values(v).size() == 1) << Volkszaehler_Test::values(v).size();
	ASSERT_EQ(Volkszaehler_Test::values(v).front(), r7);
	Volkszaehler_Test::values(v).pop_front();

	json_object_put(j);
	ASSERT_TRUE(ch->buffer()->size() == 0);
}
