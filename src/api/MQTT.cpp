/***********************************************************************/
/** @file MQTT.cpp
 * Implementation of the per-channel MQTT api (issue #550).
 *
 * The broker connection is owned by the global MqttClient (mqtt.hpp), which is
 * created from the global "mqtt" config block. This class consumes the channel
 * buffer in send() - just like the other apis - so aggregation and duplicate
 * handling apply before values are published.
 *
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
 * @package vzlogger
 * @license http://opensource.org/licenses/gpl-license.php GNU Public License
 **/
/*---------------------------------------------------------------------*/

/*
 * This file is part of volkzaehler.org
 *
 * volkzaehler.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * volkzaehler.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with volkszaehler.org. If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h> // pulls config.hpp which defines ENABLE_MQTT

#ifdef ENABLE_MQTT

#include <api/MQTT.hpp>

#include "mqtt.hpp"
#include <json-c/json.h>

vz::api::MQTT::MQTT(const Channel::Ptr &ch, const std::list<Option> &pOptions)
	: ApiIF(ch), _sendAgg(false), _sendRaw(true), _qos(0), _retain(false), _timestamp(false),
	  _announced(false), _last_timestamp(0), _lastReadingSent(0) {
	OptionList optlist;
	print(log_debug, "MQTT API initialize", ch->name());

	// defaults are taken from the global "mqtt" config block (broker connection),
	// per-channel options may override them.
	if (mqttClient) {
		_qos = mqttClient->qos();
		_retain = mqttClient->retain();
		_timestamp = mqttClient->timestamp();
	}

	// optional per-channel overrides:
	try {
		_qos = optlist.lookup_int(pOptions, "qos");
		if (_qos < 0 || _qos > 2) {
			print(log_alert, "api MQTT ignoring invalid qos %d", ch->name(), _qos);
			_qos = mqttClient ? mqttClient->qos() : 0;
		}
	} catch (vz::OptionNotFoundException &e) {
	} catch (vz::VZException &e) {
		print(log_alert, "api MQTT requires parameter \"qos\" as int!", ch->name());
		throw;
	}

	try {
		_retain = optlist.lookup_bool(pOptions, "retain");
	} catch (vz::OptionNotFoundException &e) {
	} catch (vz::VZException &e) {
		print(log_alert, "api MQTT requires parameter \"retain\" as bool!", ch->name());
		throw;
	}

	try {
		_timestamp = optlist.lookup_bool(pOptions, "timestamp");
	} catch (vz::OptionNotFoundException &e) {
	} catch (vz::VZException &e) {
		print(log_alert, "api MQTT requires parameter \"timestamp\" as bool!", ch->name());
		throw;
	}

	// determine base topic: either an explicit per-channel "topic" or the global
	// prefix + channel name (keeps the topic scheme of the previous implementation).
	std::string base;
	try {
		base = optlist.lookup_string(pOptions, "topic");
		// strip a trailing '/' so we can append the raw/agg suffix consistently
		if (!base.empty() && base.back() == '/')
			base.pop_back();
		base += '/';
	} catch (vz::OptionNotFoundException &e) {
		base = mqttClient ? mqttClient->topicPrefix() : std::string("vzlogger/");
		base += ch->name();
		base += '/';
	} catch (vz::VZException &e) {
		print(log_alert, "api MQTT requires parameter \"topic\" as string!", ch->name());
		throw;
	}

	_announcePrefix = base;
	_topicRaw = base + "raw";
	_topicAgg = base + "agg";

	// publish to the agg topic if the channel aggregates, otherwise to the raw topic.
	_sendAgg = ch->buffer() && ch->buffer()->get_aggmode() != Buffer::aggmode::NONE;
	if (_sendAgg && !(mqttClient && mqttClient->rawAndAgg()))
		_sendRaw = false;

	// build announcement values (uuid, identifier) published once in register_device():
	if (ch->identifier()) {
		char unparseBuf[200];
		unparseBuf[0] = 0;
		if (ch->identifier()->unparse(unparseBuf, sizeof(unparseBuf))) {
			_announceValues.emplace_back("id", unparseBuf);
		}
	}
	std::string uuid = ch->uuid();
	if (uuid.length())
		_announceValues.emplace_back("uuid", uuid);

	print(log_debug, "api MQTT using topics raw=%s agg=%s (sendRaw=%d sendAgg=%d)", ch->name(),
		  _topicRaw.c_str(), _topicAgg.c_str(), _sendRaw, _sendAgg);
}

vz::api::MQTT::~MQTT() {
	if (_lastReadingSent)
		delete _lastReadingSent;
}

std::string vz::api::MQTT::makePayload(int64_t timestamp, double value) const {
	if (_timestamp) {
		struct json_object *payload_obj = json_object_new_object();
		json_object_object_add(payload_obj, "timestamp", json_object_new_int64(timestamp));
		json_object_object_add(payload_obj, "value", json_object_new_double(value));
		std::string payload = json_object_to_json_string(payload_obj);
		json_object_put(payload_obj);
		return payload;
	}
	return std::to_string(value);
}

void vz::api::MQTT::register_device() {
	// announce uuid/identifier once (if connected):
	if (!mqttClient || _announced)
		return;
	for (auto &v : _announceValues) {
		mqttClient->publish(_announcePrefix + v.first, v.second);
	}
	_announced = true;
}

void vz::api::MQTT::send() {
	Buffer::Ptr buf = channel()->buffer();
	Buffer::iterator it;

	// make sure the announcement happened (register_device() is also called at
	// startup, but a (re)connect may have occurred since then).
	register_device();

	const std::string &topic = _sendAgg ? _topicAgg : _topicRaw;

	// nothing configured to publish, but we must still drain the buffer so it
	// does not grow unbounded.
	const bool publishing = (mqttClient != 0) && (_sendAgg || _sendRaw);

	int64_t timestamp = 1;
	const int duplicates = channel()->duplicates();
	const int duplicates_ms = duplicates * 1000;
	int sent = 0;

	buf->lock();
	for (it = buf->begin(); it != buf->end(); it++) {
		bool sendData = false;
		timestamp = it->time_ms();

		// only consider readings that are not older than the last one we sent:
		if (_last_timestamp <= timestamp) {
			if (0 == duplicates) { // send all values
				sendData = true;
				_last_timestamp = timestamp;
			} else {
				const Reading &r = *it;
				// duplicates should be ignored, but send at least each <duplicates> seconds
				if (!_lastReadingSent) {
					sendData = true;
					_lastReadingSent = new Reading(r);
					_last_timestamp = timestamp;
				} else if ((timestamp >= (_last_timestamp + duplicates_ms)) ||
						   (r.value() != _lastReadingSent->value())) {
					sendData = true;
					_last_timestamp = timestamp;
					*_lastReadingSent = r;
				}
			}
		}

		if (sendData && publishing) {
			mqttClient->publish(topic, makePayload(timestamp, it->value()));
			sent++;
		}

		it->mark_delete();
	}
	buf->unlock();
	buf->clean();

	if (!publishing) {
		print(log_finest, "api MQTT nothing published (no broker connection or no topic)",
			  channel()->name());
	} else {
		print(log_debug, "api MQTT published %d readings to %s", channel()->name(), sent,
			  topic.c_str());
	}
}

#endif // ENABLE_MQTT
