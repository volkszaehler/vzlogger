/*
 * Home Assistant MQTT Discovery helpers — see include/mqtt_ha_discovery.hpp.
 */

#include "mqtt_ha_discovery.hpp"
#include <json-c/json.h>
#include <unordered_map>

namespace vz {
namespace mqtt_ha {

namespace {

// Mapping from OBIS identifiers (as produced by ObisIdentifier::unparse) to
// Home Assistant sensor metadata. Extend as needed; unknown OBIS codes return
// nullptr and the caller falls back to a unit-less generic sensor.
const std::unordered_map<std::string, ObisHaMeta> &obisHaMap() {
	static const std::unordered_map<std::string, ObisHaMeta> map = {
		{"1-0:1.8.0*255", {"Wh", "energy", "total_increasing", "Active energy import (total)"}},
		{"1-0:1.8.1*255", {"Wh", "energy", "total_increasing", "Active energy import (tariff 1)"}},
		{"1-0:1.8.2*255", {"Wh", "energy", "total_increasing", "Active energy import (tariff 2)"}},
		{"1-0:2.8.0*255", {"Wh", "energy", "total_increasing", "Active energy export (total)"}},
		{"1-0:2.8.1*255", {"Wh", "energy", "total_increasing", "Active energy export (tariff 1)"}},
		{"1-0:2.8.2*255", {"Wh", "energy", "total_increasing", "Active energy export (tariff 2)"}},
		{"1-0:16.7.0*255", {"W", "power", "measurement", "Active power (sum)"}},
		{"1-0:1.7.0*255", {"W", "power", "measurement", "Active power import"}},
		{"1-0:2.7.0*255", {"W", "power", "measurement", "Active power export"}},
		{"1-0:36.7.0*255", {"W", "power", "measurement", "Active power L1"}},
		{"1-0:56.7.0*255", {"W", "power", "measurement", "Active power L2"}},
		{"1-0:76.7.0*255", {"W", "power", "measurement", "Active power L3"}},
		{"1-0:32.7.0*255", {"V", "voltage", "measurement", "Voltage L1"}},
		{"1-0:52.7.0*255", {"V", "voltage", "measurement", "Voltage L2"}},
		{"1-0:72.7.0*255", {"V", "voltage", "measurement", "Voltage L3"}},
		{"1-0:31.7.0*255", {"A", "current", "measurement", "Current L1"}},
		{"1-0:51.7.0*255", {"A", "current", "measurement", "Current L2"}},
		{"1-0:71.7.0*255", {"A", "current", "measurement", "Current L3"}},
	};
	return map;
}

} // namespace

const ObisHaMeta *lookupObisHa(const std::string &obisKey) {
	const auto &map = obisHaMap();
	auto it = map.find(obisKey);
	return it == map.end() ? nullptr : &it->second;
}

struct json_object *buildDiscoveryConfig(const std::string &uniqueId, const std::string &sensorName,
										 const std::string &stateTopic, bool timestampPayload,
										 const ObisHaMeta *meta,
										 const std::string &deviceIdentifier,
										 const std::string &deviceName) {
	struct json_object *cfg = json_object_new_object();

	json_object_object_add(cfg, "name", json_object_new_string(sensorName.c_str()));
	json_object_object_add(cfg, "unique_id", json_object_new_string(uniqueId.c_str()));
	json_object_object_add(cfg, "object_id", json_object_new_string(uniqueId.c_str()));
	json_object_object_add(cfg, "state_topic", json_object_new_string(stateTopic.c_str()));

	if (timestampPayload) {
		json_object_object_add(cfg, "value_template",
							   json_object_new_string("{{ value_json.value }}"));
	}

	if (meta) {
		if (meta->unit)
			json_object_object_add(cfg, "unit_of_measurement", json_object_new_string(meta->unit));
		if (meta->deviceClass)
			json_object_object_add(cfg, "device_class", json_object_new_string(meta->deviceClass));
		if (meta->stateClass)
			json_object_object_add(cfg, "state_class", json_object_new_string(meta->stateClass));
	}

	struct json_object *device = json_object_new_object();
	struct json_object *identifiers = json_object_new_array();
	json_object_array_add(identifiers, json_object_new_string(deviceIdentifier.c_str()));
	json_object_object_add(device, "identifiers", identifiers);
	json_object_object_add(device, "name", json_object_new_string(deviceName.c_str()));
	json_object_object_add(device, "manufacturer", json_object_new_string("volkszaehler.org"));
	json_object_object_add(device, "model", json_object_new_string("vzlogger"));
	json_object_object_add(cfg, "device", device);

	return cfg;
}

} // namespace mqtt_ha
} // namespace vz
