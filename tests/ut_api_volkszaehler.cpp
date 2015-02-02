#include "gtest/gtest.h"
#include "api/Volkszaehler.hpp"
// #include "api/CurlResponse.hpp"

// dirty hack until we find a better solution:
#include "../src/api/Volkszaehler.cpp"
#include "../src/Buffer.cpp"
#include "../src/Config_Options.cpp"
#include "../src/Channel.cpp"

// another dirty hack to test private functions from Volkszaehler:
namespace vz {
namespace api{
class Volkszaehler_Test
{
	public:
		static void api_parse_exception(vz::api::Volkszaehler &v, vz::api::CURLresponse &r, 
		char *&err, size_t &n){ v.api_parse_exception(r, err, n);} 
		static std::list<Reading> &values(Volkszaehler &v) {return v._values;}
};
}
}

Config_Options options("etc/vzlogger.conf");

TEST(api_Volkszaehler, config_options){
	MapContainer mappings;
#ifdef OCR_SUPPORT
	options.config_parse(mappings); // let's see whether we can parse the provided example config
#else
	ASSERT_THROW(options.config_parse(mappings), vz::VZException); // the default config should fail due to missing Protocol ocr. There might be another failure...
#endif
}

TEST(api_Volkszaehler, api_parse_exception) {
using namespace vz::api;
	CURLresponse resp;
	resp.data = 0;
	resp.size = 0;
	char *err = new char[100];
	err[0]=0;
	size_t n=100;
	std::list<Option> options;
	options.push_front(Option("middleware", (char*)"bla_middleware"));
	ReadingIdentifier::Ptr pRid;
	Channel *ch = new Channel(options, std::string("bla_api"), std::string("bla_uuid"), pRid);
	Channel::Ptr chp(ch);
	Volkszaehler v(chp, options);
	
	// test using empty data:
	// todo bug: this writes in err even if its null!
	Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	ASSERT_STREQ("continue", err ) << err;
	
	// test using small size for n:
	n=5;
	Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	// todo bug: doesn't zero terminate the string! ASSERT_STREQ("conti", err ) << err;
	n=100;
	
	// test using corrupted resp data:
	resp.size = 100;
	resp.data = 0;
	// todo: this crashes! Volkszaehler_Test::api_parse_exception(v, resp, err, n);

	// test using empty json resp:
	resp.data = (char*)"{}%";
	resp.size = strlen(resp.data);
	Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	ASSERT_STREQ("Missing exception", err);
	
	// test using just exception:
	resp.data = (char*)"{exception}%";
	resp.size = strlen(resp.data);
	Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	ASSERT_STREQ("quoted object property name expected", err);
	
	// test using just "exception":
	resp.data = (char*)"{\"exception\"}%";
	resp.size = strlen(resp.data);
	Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	ASSERT_STREQ("object property name separator ':' expected", err);

	// test using proper "exception":
	resp.data = (char*)"{\"exception\":\"1\"}%";
	resp.size = strlen(resp.data);
	// todo bug throws a c++ exception (null not valid) Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	// ASSERT_STREQ("object property name separator ':' expected", err);
	
	// test using proper "exception" but not type/message:
	resp.data = (char*)"{\"exception\": { \"type\":\"1\", \"message\":\"2\"  } }%";
	resp.size = strlen(resp.data);
	Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	ASSERT_STREQ("'1': '2'", err);	
	
	// test type: "UniqueConstraintViolationException" message contains "Duplicate entry"
	resp.data = (char*)"{\"exception\": { \"type\":\"UniqueConstraintViolationException\", \"message\":\"2 Duplicate entry\"  } }%";
	resp.size = strlen(resp.data);
	// todo bug: crashes with double free if _values is empty! Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	//ASSERT_STREQ("'UniqueConstraintViolationException': '2 Duplicate entry'", err);	
	Volkszaehler_Test::values(v).push_front(Reading());
	Volkszaehler_Test::api_parse_exception(v, resp, err, n);
	ASSERT_TRUE(0 == Volkszaehler_Test::values(v).size());
	ASSERT_STREQ("'UniqueConstraintViolationException': '2 Duplicate entry'", err);	
	
	delete [] err;
}

