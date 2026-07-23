/*
 * Author: Matthias Behr, mbehr (a) mcbehr dot de
 * (c) 2018
 * */

#include "mqtt.hpp"
#include "common.h"
#include "mosquitto.h"
#include "mqtt_ha_discovery.hpp"
#include <cassert>
#include <sstream>
#include <unistd.h>

// global var:
MqttClient *mqttClient = 0;
volatile bool endMqttClientThread = false;

// class impl.
MqttClient::MqttClient(struct json_object *option) : _enabled(false) {

	print(log_finest, "MqttClient::MqttClient called", "mqtt");
	if (option) {
		assert(json_object_get_type(option) == json_type_object);
		json_object_object_foreach(option, key, local_value) {
			enum json_type local_type = json_object_get_type(local_value);

			if (strcmp(key, "enabled") == 0 && local_type == json_type_boolean) {
				_enabled = json_object_get_boolean(local_value);
			} else if (strcmp(key, "retain") == 0 && local_type == json_type_boolean) {
				_retain = json_object_get_boolean(local_value);
			} else if (strcmp(key, "rawAndAgg") == 0 && local_type == json_type_boolean) {
				_rawAndAgg = json_object_get_boolean(local_value);
			} else if (strcmp(key, "port") == 0 && local_type == json_type_int) {
				_port = json_object_get_int(local_value);
			} else if (strcmp(key, "keepalive") == 0 && local_type == json_type_int) {
				_keepalive = json_object_get_int(local_value);
			} else if (strcmp(key, "host") == 0 && local_type == json_type_string) {
				_host = json_object_get_string(local_value);
			} else if (strcmp(key, "user") == 0 && local_type == json_type_string) {
				_user = json_object_get_string(local_value);
			} else if (strcmp(key, "pass") == 0 && local_type == json_type_string) {
				_pwd = json_object_get_string(local_value);
			} else if (strcmp(key, "cafile") == 0 && local_type == json_type_string) {
				_cafile = json_object_get_string(local_value);
			} else if (strcmp(key, "capath") == 0 && local_type == json_type_string) {
				_capath = json_object_get_string(local_value);
			} else if (strcmp(key, "certfile") == 0 && local_type == json_type_string) {
				_certfile = json_object_get_string(local_value);
			} else if (strcmp(key, "keyfile") == 0 && local_type == json_type_string) {
				_keyfile = json_object_get_string(local_value);
			} else if (strcmp(key, "keypass") == 0 && local_type == json_type_string) {
				_keypass = json_object_get_string(local_value);
			} else if (strcmp(key, "topic") == 0 && local_type == json_type_string) {
				_topic = json_object_get_string(local_value);
				// todo check for no $ at start and no / at end
			} else if (strcmp(key, "qos") == 0 && local_type == json_type_int) {
				int qos = json_object_get_int(local_value);
				if (qos >= 0 && qos <= 2) {
					_qos = qos;
				} else {
					print(log_alert, "Ignoring invalid QoS value %d, assuming default", NULL, qos);
				}
			} else if (strcmp(key, "timestamp") == 0 && local_type == json_type_boolean) {
				_timestamp = json_object_get_boolean(local_value);
			} else if (strcmp(key, "id") == 0 && local_type == json_type_string) {
				_id = json_object_get_string(local_value);
			} else if (strcmp(key, "ha_discovery") == 0 && local_type == json_type_object) {
				parseHaDiscoveryConfig(local_value);
			} else {
				print(log_alert, "Ignoring invalid field or type: %s=%s", NULL, key,
					  json_object_get_string(local_value));
			}
		}
	} else {
		throw vz::VZException("config: mqtt no options!");
	}

	// default topic to vzlogger
	if (!_topic.length())
		_topic = std::string("vzlogger/");
	else
		_topic += '/';

	// HA discovery defaults
	if (_haEnabled) {
		if (!_haPrefix.length())
			_haPrefix = "homeassistant";
		if (!_haDeviceName.length())
			_haDeviceName = "vzlogger";
		if (!_haDeviceIdentifier.length())
			_haDeviceIdentifier = _id.length() ? _id : _haDeviceName;
	}

	// mosquitto lib init:
	if (mosquitto_lib_init() != MOSQ_ERR_SUCCESS) {
		print(log_alert, "libmosquitto init failed! Stopped.", "mqtt");
		_enabled = false;
	}

	// check mosquitto version:
	int major = -1;
	int minor = -1;
	int revision = -1;
	mosquitto_lib_version(&major, &minor, &revision); // tested with 1.4.15
	print(log_finest, "using libmosquitto %d.%d.%d", "mqtt", major, minor, revision);
	if (major != LIBMOSQUITTO_MAJOR) {
		print(log_alert, "Wrong libmosquitto major version! %d vs. expected %d! Stopped.", "mqtt",
			  major, LIBMOSQUITTO_MAJOR);
		_enabled = false;
	}

	if (isConfigured()) {
		std::ostringstream id;

		if (!_id.length()) {
			// create an id. for now just use the pid.
			id << "vzlogger_" << getpid(); // todo does this need to be in sync with topic?
		} else {
			// use id provided in configuration
			id.str(_id);
		}

		_mcs = mosquitto_new(id.str().c_str(), true, this);
		if (!_mcs) {
			print(log_alert, "mosquitto_new failed! Stopped!", "mqtt");
			_enabled = false;
		} else {
			// tell mosquitto that it should be thread safe:
			int res = mosquitto_threaded_set(_mcs, true);
			if (res != MOSQ_ERR_SUCCESS) {
				print(log_warning, "mosquitto_threaded_set failed: %s", "mqtt",
					  mosquitto_strerror(res));
			}
			// set username&pwd
			if ((_user.length() or _pwd.length()) and
				mosquitto_username_pw_set(_mcs, _user.c_str(), _pwd.c_str()) != MOSQ_ERR_SUCCESS) {
				print(log_warning, "mosquitto_username_pw_set failed! Continuing anyhow.", "mqtt");
			}

			if ((_cafile.length() or _capath.length()) or
				(_certfile.length() and _keyfile.length())) {
				// try to set tls:
				static const std::string keypass = _keypass;
				res = mosquitto_tls_set(
					_mcs, _cafile.length() ? _cafile.c_str() : nullptr,
					_capath.length() ? _capath.c_str() : nullptr,
					_certfile.length() ? _certfile.c_str() : nullptr,
					_keyfile.length() ? _keyfile.c_str() : nullptr,
					[](char *buf, int size, int rwflag, void *userdata) {
						int len = static_cast<int>(keypass.length());
						print(log_finest,
							  "mosquitto_tls_set onpasswd (size=%d) called. keypass len=%d", "mqtt",
							  size, len);
						if (len > size)
							len = size;
						if (len)
							memcpy(buf, keypass.data(), len);
						return len;
					});
				if (res != MOSQ_ERR_SUCCESS) {
					print(log_warning, "mosquitto_tls_set failed: %s", "mqtt",
						  mosquitto_strerror(res));
				}
			}

			int protocol = MQTT_PROTOCOL_V311;
			res = mosquitto_opts_set(_mcs, MOSQ_OPT_PROTOCOL_VERSION, &protocol);
			if (res != MOSQ_ERR_SUCCESS) {
				print(log_warning, "unable to set MQTT protocol version (error %d)", "mqtt", res);
			}

			mosquitto_connect_callback_set(_mcs, [](struct mosquitto *mosq, void *obj, int res) {
				static_cast<MqttClient *>(obj)->connect_callback(mosq, res);
			});
			mosquitto_disconnect_callback_set(_mcs, [](struct mosquitto *mosq, void *obj, int res) {
				static_cast<MqttClient *>(obj)->disconnect_callback(mosq, res);
			});

			mosquitto_message_callback_set(
				_mcs, [](struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
					static_cast<MqttClient *>(obj)->message_callback(mosq, msg);
				});

			// now connect. we use sync interface with spe. thread calling mosquitto_loop
			res = mosquitto_connect(_mcs, _host.c_str(), _port, _keepalive);
			if (res != MOSQ_ERR_SUCCESS) {
				switch (res) {
				case MOSQ_ERR_CONN_REFUSED:
				case MOSQ_ERR_ERRNO:
					// We assume all system errors to be externally recoverable,
					// so we retry.
					print(log_warning, "mosquitto_connect failed (but trying anyhow): %s", "mqtt",
						  mosquitto_strerror(res));
					break;
				default:
					// This should be MOSQ_ERR_INVAL, so there is nothing we can do.
					print(log_alert, "mosquitto_connect failed, giving up: %s", "mqtt",
						  mosquitto_strerror(res));
					_enabled = false;
					break;
				}
			}
		}
	}
}

bool MqttClient::isConfigured() const {
	if (!_enabled)
		return false;
	if (!_port)
		print(log_finest, "mqtt port not configured!", "mqtt");
	if (!_host.length())
		print(log_warning, "mqtt host not configured!", "mqtt");

	return _port != 0 and _host.length();
}

MqttClient::~MqttClient() {
	print(log_finest, "~MqttClient called", "mqtt");

	if (_mcs) {
		assert(!_enabled or endMqttClientThread);

		mosquitto_disconnect(_mcs);
		// we call mosquitto_loop at least once here as the thread should be stopped already:
		int res = mosquitto_loop(_mcs, 50, 1);
		if (res != MOSQ_ERR_SUCCESS and res != MOSQ_ERR_NO_CONN) {
			print(log_warning, "mosquitto_loop returned: %s", "mqtt", mosquitto_strerror(res));
		}
		mosquitto_destroy(_mcs);
	}
	mosquitto_lib_cleanup(); // this assumes nobody else is using libmosquitto!
}

void MqttClient::parseHaDiscoveryConfig(struct json_object *option) {
	if (!option)
		return;
	json_object_object_foreach(option, key, value) {
		enum json_type type = json_object_get_type(value);
		if (strcmp(key, "enabled") == 0 && type == json_type_boolean) {
			_haEnabled = json_object_get_boolean(value);
		} else if (strcmp(key, "prefix") == 0 && type == json_type_string) {
			_haPrefix = json_object_get_string(value);
		} else if (strcmp(key, "device_name") == 0 && type == json_type_string) {
			_haDeviceName = json_object_get_string(value);
		} else if (strcmp(key, "device_identifier") == 0 && type == json_type_string) {
			_haDeviceIdentifier = json_object_get_string(value);
		} else {
			print(log_alert, "Ignoring invalid ha_discovery field: %s", NULL, key);
		}
	}
}

void MqttClient::publishHaDiscovery(Channel &ch, const std::string &stateTopic, bool aggregated) {
	if (!_mcs)
		return;

	// Resolve OBIS identifier (if any) for metadata lookup.
	std::string obisKey;
	char unparseBuf[200] = {0};
	if (ch.identifier() && ch.identifier()->unparse(unparseBuf, sizeof(unparseBuf))) {
		obisKey = unparseBuf;
	}
	const vz::mqtt_ha::ObisHaMeta *meta = vz::mqtt_ha::lookupObisHa(obisKey);

	// Unique id derived from channel UUID (dashes stripped for an entity-id friendly slug).
	std::string uuid = ch.uuid() ? ch.uuid() : "";
	std::string slug;
	slug.reserve(uuid.size());
	for (char c : uuid) {
		if (c != '-')
			slug.push_back(c);
	}
	if (slug.empty())
		slug = ch.name() ? ch.name() : "channel";
	const std::string uniqueId = "vzlogger_" + slug;

	std::string sensorName = meta ? meta->defaultName : (obisKey.length() ? obisKey : uniqueId);
	if (aggregated)
		sensorName += " (agg)";

	struct json_object *cfg = vz::mqtt_ha::buildDiscoveryConfig(
		uniqueId, sensorName, stateTopic, _timestamp, meta, _haDeviceIdentifier, _haDeviceName);

	const std::string discoveryTopic = _haPrefix + "/sensor/" + uniqueId + "/config";
	const char *payload = json_object_to_json_string(cfg);
	int payloadLen = static_cast<int>(strlen(payload));
	// HA Discovery requires the config topic to be retained so HA picks it up on reconnect.
	int res = mosquitto_publish(_mcs, 0, discoveryTopic.c_str(), payloadLen, payload, _qos, true);
	if (res != MOSQ_ERR_SUCCESS) {
		print(log_warning, "HA discovery publish to %s failed: %s", "mqtt", discoveryTopic.c_str(),
			  mosquitto_strerror(res));
	} else {
		print(log_finest, "HA discovery published %s -> state=%s", "mqtt", discoveryTopic.c_str(),
			  stateTopic.c_str());
	}

	json_object_put(cfg);
}

void MqttClient::ChannelEntry::generateNames(const std::string &prefix, Channel &ch) {
	_announceValues.clear();
	_fullTopicRaw = prefix;
	// Use configured mqtt_topic if set, otherwise fall back to ch.name()
	_fullTopicRaw += (!ch.mqttTopic().empty()) ? ch.mqttTopic() : ch.name();
	_fullTopicRaw += '/';
	if (ch.identifier()) {
		char unparseBuf[200];
		unparseBuf[0] = 0;
		if (ch.identifier()->unparse(unparseBuf, sizeof(unparseBuf))) {
			_announceValues.emplace_back("id", unparseBuf);
		}
		print(log_finest, "generateNames: ch.name()=%s ch.identifier.toString==%s unparse=%s",
			  "mqtt", ch.name(), ch.identifier()->toString().c_str(), unparseBuf);
	}
	_sendAgg = ch.buffer() && ch.buffer()->get_aggmode() != Buffer::aggmode::NONE;

	_fullTopicAgg = _fullTopicRaw;
	_announceName = _fullTopicRaw;

	_fullTopicRaw += "raw";
	_fullTopicAgg += "agg";
	std::string uuid = ch.uuid();
	if (uuid.length())
		_announceValues.emplace_back("uuid", uuid);
}

void MqttClient::publish(Channel::Ptr ch, Reading &rds, bool aggregate) {
	// take care: this function must be thread safe and non-blocking!
	// for now we do only call this from read_thread and our mqtt_client thread doesn't harm here
	// mosquitto_publish doesn't seem to be blocking. needs further investigation!

	if (!ch)
		return;
	if (!_mcs)
		return;

	// search for cached values:
	std::unique_lock<std::mutex> lock(_chMapMutex);
	auto it = _chMap.find(ch->name());
	if (it == _chMap.end()) {
		ChannelEntry entry;
		entry.generateNames(_topic, (*ch));
		if (entry._sendAgg && !_rawAndAgg)
			entry._sendRaw = false;

		it = _chMap.emplace(std::make_pair(ch->name(), entry)).first;
	}

	assert(it != _chMap.end());
	ChannelEntry &entry = (*it).second;
	// do we need to announce the uuid?
	if (!entry._announced && entry._announceValues.size()) {
		for (auto &v : entry._announceValues) {
			std::string name = entry._announceName + v.first;
			int res = mosquitto_publish(_mcs, 0, name.c_str(), v.second.length(), v.second.c_str(),
										_qos, _retain);
			if (res != MOSQ_ERR_SUCCESS) {
				print(log_finest, "mosquitto_publish announce \"%s\" failed: %s", "mqtt",
					  name.c_str(), mosquitto_strerror(res));
			} else {
				entry._announced = true; // if one can be announced we treat it successfull
			}
		}
	}

	// Publish Home Assistant MQTT Discovery config once per channel. The state_topic
	// follows whichever data stream is actually being emitted (agg-only vs. raw).
	if (_haEnabled && !entry._haAnnounced) {
		const bool aggOnly = entry._sendAgg && !entry._sendRaw;
		const std::string &stateTopic = aggOnly ? entry._fullTopicAgg : entry._fullTopicRaw;
		publishHaDiscovery(*ch, stateTopic, aggOnly);
		entry._haAnnounced = true;
	}

	std::string &topic = aggregate ? entry._fullTopicAgg : entry._fullTopicRaw;
	if ((entry._sendAgg and aggregate) or (entry._sendRaw && !aggregate)) {
		lock.unlock(); // we can unlock here already
		std::string payload;
		struct json_object *payload_obj = NULL;

		if (_timestamp) {
			payload_obj = json_object_new_object();
			json_object_object_add(payload_obj, "timestamp", json_object_new_int64(rds.time_ms()));
			json_object_object_add(payload_obj, "value", json_object_new_double(rds.value()));
			payload = json_object_to_json_string(payload_obj);
		} else {
			payload = std::to_string(rds.value());
		}

		print(log_finest, "publish %s=%s", "mqtt", topic.c_str(), payload.c_str());

		int res = mosquitto_publish(_mcs, 0, topic.c_str(), payload.length(), payload.c_str(), _qos,
									_retain);
		if (res != MOSQ_ERR_SUCCESS) {
			print(log_finest, "mosquitto_publish failed: %s", "mqtt", mosquitto_strerror(res));
		}
		if (payload_obj != NULL) {
			json_object_put(payload_obj);
		}
	}
}

void MqttClient::connect_callback(struct mosquitto *mosq, int result) {
	print(log_finest, "connect_callback called, res=%d", "mqtt", result);
	switch (result) {
	case MOSQ_ERR_SUCCESS:
		_isConnected = true;
		break;
	default:
		_isConnected = false;
		break;
	}
}

void MqttClient::disconnect_callback(struct mosquitto *mosq, int result) {
	print(log_finest, "disconnect_callback called, res=%d", "mqtt", result);
	_isConnected = false;
}

void MqttClient::message_callback(struct mosquitto *mosq, const struct mosquitto_message *msg) {
	print(log_finest, "message_callback called", "mqtt");
}

void *mqtt_client_thread(void *arg) {
	print(log_debug, "Start mqtt_client_thread", "mqtt");

	if (mqttClient) {
		while (!endMqttClientThread) {
			int res = mosquitto_loop(mqttClient->_mcs, 1000, 1);
			if (res != MOSQ_ERR_SUCCESS) {
				print(log_warning, "mosquitto_loop failed (trying to reconnect): %s", "mqtt",
					  mosquitto_strerror(res));
				sleep(1);
				res = mosquitto_reconnect(mqttClient->_mcs);
				if (res != MOSQ_ERR_SUCCESS) {
					print(log_warning, "mosquitto_reconnect failed: %s", "mqtt",
						  mosquitto_strerror(res));
					// todo investigate: will next loop return same != SUCCESS and trigger recon?
				} else {
					print(log_finest, "mosquitto_reconnect succeeded", "mqtt");
				}
			}
		}
	}

	print(log_debug, "Stopped mqtt_client_thread", "mqtt");
	return 0;
}

void end_mqtt_client_thread() { endMqttClientThread = true; }
