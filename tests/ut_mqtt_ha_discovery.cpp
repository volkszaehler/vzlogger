#include "mqtt_ha_discovery.hpp"
#include "gtest/gtest.h"
#include <json-c/json.h>
#include <string>

using vz::mqtt_ha::buildDiscoveryConfig;
using vz::mqtt_ha::lookupObisHa;
using vz::mqtt_ha::ObisHaMeta;

namespace {

std::string jsonField(json_object *obj, const char *key) {
	json_object *v = nullptr;
	if (!json_object_object_get_ex(obj, key, &v))
		return "";
	return json_object_get_string(v);
}

bool jsonHas(json_object *obj, const char *key) {
	json_object *v = nullptr;
	return json_object_object_get_ex(obj, key, &v);
}

} // namespace

TEST(MqttHaDiscovery, LookupKnownEnergyImport) {
	const ObisHaMeta *m = lookupObisHa("1-0:1.8.0*255");
	ASSERT_NE(m, nullptr);
	EXPECT_STREQ(m->unit, "Wh");
	EXPECT_STREQ(m->deviceClass, "energy");
	EXPECT_STREQ(m->stateClass, "total_increasing");
}

TEST(MqttHaDiscovery, LookupKnownActivePower) {
	const ObisHaMeta *m = lookupObisHa("1-0:16.7.0*255");
	ASSERT_NE(m, nullptr);
	EXPECT_STREQ(m->unit, "W");
	EXPECT_STREQ(m->deviceClass, "power");
	EXPECT_STREQ(m->stateClass, "measurement");
}

TEST(MqttHaDiscovery, LookupUnknownReturnsNull) {
	EXPECT_EQ(lookupObisHa("1-0:99.99.99*255"), nullptr);
	EXPECT_EQ(lookupObisHa(""), nullptr);
}

TEST(MqttHaDiscovery, BuildConfigWithKnownObis) {
	const ObisHaMeta *m = lookupObisHa("1-0:1.8.0*255");
	ASSERT_NE(m, nullptr);

	json_object *cfg = buildDiscoveryConfig("vzlogger_abc", "Bezug", "vzlogger/chn0/raw",
											/*timestampPayload=*/false, m, "vzlogger", "vzlogger");
	ASSERT_NE(cfg, nullptr);

	EXPECT_EQ(jsonField(cfg, "name"), "Bezug");
	EXPECT_EQ(jsonField(cfg, "unique_id"), "vzlogger_abc");
	EXPECT_EQ(jsonField(cfg, "object_id"), "vzlogger_abc");
	EXPECT_EQ(jsonField(cfg, "state_topic"), "vzlogger/chn0/raw");
	EXPECT_EQ(jsonField(cfg, "unit_of_measurement"), "Wh");
	EXPECT_EQ(jsonField(cfg, "device_class"), "energy");
	EXPECT_EQ(jsonField(cfg, "state_class"), "total_increasing");
	EXPECT_FALSE(jsonHas(cfg, "value_template"));

	json_object *device = nullptr;
	ASSERT_TRUE(json_object_object_get_ex(cfg, "device", &device));
	EXPECT_EQ(jsonField(device, "name"), "vzlogger");
	EXPECT_EQ(jsonField(device, "manufacturer"), "volkszaehler.org");
	EXPECT_EQ(jsonField(device, "model"), "vzlogger");

	json_object *ids = nullptr;
	ASSERT_TRUE(json_object_object_get_ex(device, "identifiers", &ids));
	ASSERT_EQ(json_object_get_type(ids), json_type_array);
	ASSERT_EQ(json_object_array_length(ids), 1);
	EXPECT_STREQ(json_object_get_string(json_object_array_get_idx(ids, 0)), "vzlogger");

	json_object_put(cfg);
}

TEST(MqttHaDiscovery, BuildConfigWithoutMetaOmitsUnit) {
	json_object *cfg =
		buildDiscoveryConfig("vzlogger_x", "x", "vzlogger/chn0/raw",
							 /*timestampPayload=*/false, /*meta=*/nullptr, "vzlogger", "vzlogger");
	ASSERT_NE(cfg, nullptr);

	EXPECT_FALSE(jsonHas(cfg, "unit_of_measurement"));
	EXPECT_FALSE(jsonHas(cfg, "device_class"));
	EXPECT_FALSE(jsonHas(cfg, "state_class"));
	EXPECT_FALSE(jsonHas(cfg, "value_template"));

	json_object_put(cfg);
}

TEST(MqttHaDiscovery, BuildConfigWithTimestampAddsValueTemplate) {
	json_object *cfg =
		buildDiscoveryConfig("vzlogger_x", "x", "vzlogger/chn0/raw",
							 /*timestampPayload=*/true, /*meta=*/nullptr, "vzlogger", "vzlogger");
	ASSERT_NE(cfg, nullptr);

	EXPECT_EQ(jsonField(cfg, "value_template"), "{{ value_json.value }}");

	json_object_put(cfg);
}
