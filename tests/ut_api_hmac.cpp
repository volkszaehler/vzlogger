
#include <api/hmac.h>

#include "gtest/gtest.h"
using namespace testing;

TEST(api_hmac, simple_digest) {

	char digest[256];
	unsigned char *data = (unsigned char *)"Test";
	size_t datalen = 4;
	unsigned char *secretkey = (unsigned char *)"secret";
	size_t secretlen = 6;

	vz::hmac_sha1(digest, data, datalen, secretkey, secretlen);
	ASSERT_STREQ(digest, "X-Digest: fdaa1009d29b3de5e4fa6b0f31226ead23e34c25");
}
