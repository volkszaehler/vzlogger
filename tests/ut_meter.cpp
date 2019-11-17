/*
 * unit tests for Meter.cpp
 * Author: Matthias Behr, 2014
 */

#include "gtest/gtest.h"

#include "Meter.hpp"

// this is a dirty hack. we should think about better ways/rules to link against the
// test objects.
// e.g. single files in the tests directory that just include the real file. This way one
// will avoid multiple includes of the same file in a different test case
// (or linking against the real file from the CMakeLists.txt)

#include "../src/Meter.cpp"
#include "../src/ltqnorm.cpp"
#include "../src/protocols/MeterExec.cpp"
#include "../src/protocols/MeterFile.cpp"
#include "../src/protocols/MeterFluksoV2.cpp"
#include "../src/protocols/MeterRandom.cpp"
#include "../src/protocols/MeterS0.cpp"

TEST(meter, meter_lookup_protocol) {
	ASSERT_EQ(ERR_NOT_FOUND, meter_lookup_protocol(NULL, NULL));
	ASSERT_EQ(SUCCESS, meter_lookup_protocol("d0", NULL));
	ASSERT_EQ(SUCCESS, meter_lookup_protocol("D0", NULL)); // case insensitive

	ASSERT_EQ(ERR_NOT_FOUND, meter_lookup_protocol("D1", NULL));

	ASSERT_EQ(SUCCESS, meter_lookup_protocol("file", NULL));
	ASSERT_EQ(SUCCESS, meter_lookup_protocol("exec", NULL));

	ASSERT_EQ(SUCCESS, meter_lookup_protocol("random", NULL));
	ASSERT_EQ(SUCCESS, meter_lookup_protocol("s0", NULL));
#ifdef SML_SUPPORT
	ASSERT_EQ(SUCCESS, meter_lookup_protocol("sml", NULL));
#endif
	ASSERT_EQ(SUCCESS, meter_lookup_protocol("fluksov2", NULL));

	ASSERT_EQ(ERR_NOT_FOUND, meter_lookup_protocol("d01", NULL));
	ASSERT_EQ(ERR_NOT_FOUND, meter_lookup_protocol("", NULL));
	meter_protocol_t m = meter_protocol_none;
	ASSERT_EQ(SUCCESS, meter_lookup_protocol("d0", &m));
	ASSERT_EQ(meter_protocol_d0, m);
	const meter_details_t *d = meter_get_details(m);
	ASSERT_TRUE(NULL != d);
	ASSERT_TRUE(NULL != d->name);
	ASSERT_STREQ(d->name, "d0");
}
