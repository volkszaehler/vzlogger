/*
 * Home Assistant MQTT Discovery helpers.
 *
 * Kept separate from mqtt.cpp / libmosquitto so the metadata lookup and
 * config-JSON assembly can be unit-tested without a broker.
 */

#ifndef __mqtt_ha_discovery_hpp_
#define __mqtt_ha_discovery_hpp_

#include <string>

struct json_object; // forward decl, defined by libjson-c

namespace vz {
namespace mqtt_ha {

struct ObisHaMeta {
	const char *unit;
	const char *deviceClass;
	const char *stateClass;
	const char *defaultName;
};

// Look up Home Assistant sensor metadata for an OBIS identifier in canonical
// form (e.g. "1-0:1.8.0*255"). Returns nullptr if the code is unknown.
const ObisHaMeta *lookupObisHa(const std::string &obisKey);

// Build a Home Assistant MQTT Discovery config JSON object. Caller takes
// ownership of the returned object and must release it with json_object_put.
//
// - meta may be nullptr (unknown OBIS) — unit/device_class/state_class are
//   then omitted.
// - When timestampPayload is true a value_template extracting `value_json.value`
//   is added; otherwise the raw payload is consumed as-is.
struct json_object *buildDiscoveryConfig(const std::string &uniqueId, const std::string &sensorName,
										 const std::string &stateTopic, bool timestampPayload,
										 const ObisHaMeta *meta,
										 const std::string &deviceIdentifier,
										 const std::string &deviceName);

} // namespace mqtt_ha
} // namespace vz

#endif
