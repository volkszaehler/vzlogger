/***********************************************************************/
/** @file MySmartGrid.cpp
 * Header file for volkszaehler.org API calls
 *
 *  @author Kai Krueger
 *  @date   2012-03-15
 *  @email  kai.krueger@itwm.fraunhofer.de
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @package vzlogger
 * @license http://opensource.org/licenses/gpl-license.php GNU Public License
 *
 * (C) Fraunhofer ITWM
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

#include <unistd.h>

#include "Config_Options.hpp"
#include <VZException.hpp>
#include <api/CurlCallback.hpp>
#include <api/MySmartGrid.hpp>
#include <api/hmac.h>

extern Config_Options options;

vz::api::MySmartGrid::MySmartGrid(Channel::Ptr ch, std::list<Option> pOptions)
	: ApiIF(ch), _channelType(chn_type_device), _scaler(1), _response(new vz::api::CurlResponse()),
	  _first_ts(0), _first_counter(0), _last_counter(0)

{
	OptionList optlist;
	print(log_debug, "===> Create MySmartGrid-API", channel()->name());
	char url[255];
	unsigned short curlTimeout = 30; // 30 seconds

	/* parse required options */
	try {
		_middleware = optlist.lookup_string(pOptions, "middleware");
		convertUuid(optlist.lookup_string(pOptions, "secretKey"), _secretKey);
		convertUuid(optlist.lookup_string(pOptions, "device"), _deviceId);

		std::string channelType = optlist.lookup_string(pOptions, "type");
		if (channelType == "device")
			_channelType = chn_type_device;
		else if (channelType == "sensor")
			_channelType = chn_type_sensor;
		else
			throw vz::VZException("Bad value for channel type.");

	} catch (vz::OptionNotFoundException &e) {
		throw;
	} catch (vz::VZException &e) {
		throw;
	}
	/* parse optional options */
	try {
		_interval = optlist.lookup_int(pOptions, "interval");
	} catch (vz::OptionNotFoundException &e) {
		_interval = 300; // default time between 2 logmessages
	} catch (vz::VZException &e) {
		throw;
	}
	try {
		_scaler = optlist.lookup_int(pOptions, "scaler");
	} catch (vz::OptionNotFoundException &e) {
		_scaler = 1; // default scaling faktor
	} catch (vz::VZException &e) {
		throw;
	}

	try {
		curlTimeout = optlist.lookup_int(pOptions, "timeout");
	} catch (vz::OptionNotFoundException &e) {
		curlTimeout = 30; // use default value instead
	} catch (vz::VZException &e) {
		throw;
	}
	convertUuid(channel()->uuid());

	switch (_channelType) {
	case chn_type_device:
		sprintf(url, "%s/device/%s", middleware().c_str(), uuid()); /* build url */
		break;
	case chn_type_sensor:
		sprintf(url, "%s/sensor/%s", middleware().c_str(), uuid()); /* build url */
		break;
	}

	print(log_debug, "msg_api_init() %s", channel()->name(), url);

	_api_header();

	curl_easy_setopt(_curlIF.handle(), CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(_curlIF.handle(), CURLOPT_SSL_VERIFYHOST, 0L);

	curl_easy_setopt(_curlIF.handle(), CURLOPT_URL, url);

	// CurlCallback::write_callback requires CurlResponse* as data
	curl_easy_setopt(_curlIF.handle(), CURLOPT_WRITEFUNCTION,
					 &(vz::api::CurlCallback::write_callback));
	curl_easy_setopt(_curlIF.handle(), CURLOPT_WRITEDATA, response());

	curl_easy_setopt(_curlIF.handle(), CURLOPT_VERBOSE, options.verbosity());

	// CurlCallback::debug_callback requires CurlResponse* as data
	curl_easy_setopt(_curlIF.handle(), CURLOPT_DEBUGFUNCTION,
					 &(vz::api::CurlCallback::debug_callback));
	curl_easy_setopt(_curlIF.handle(), CURLOPT_DEBUGDATA, response());

	// signal-handling in libcurl is NOT thread-safe. so force to deactivated them!
	curl_easy_setopt(_curlIF.handle(), CURLOPT_NOSIGNAL, 1);

	// set timeout to 5 sec. required if next router has an ip-change.
	curl_easy_setopt(_curlIF.handle(), CURLOPT_TIMEOUT, curlTimeout);
}

vz::api::MySmartGrid::~MySmartGrid() {}

void vz::api::MySmartGrid::send() {
	json_object *json_obj = NULL;
	char digest[255];

	const char *json_str;
	long int http_code;
	CURLcode curl_code;

	// check if we want to send
	time_t now = time(NULL);

	if (_first_ts > 0) {
		if ((now - first_ts()) < interval()) {
			print(log_debug, "api-MySmartGrid, skip message.", "");
			return;
		}
	} else { // _first_ts = 0
	}

	switch (_channelType) {
	case chn_type_device:
		json_obj = _apiDevice(channel()->buffer());
		break;
	case chn_type_sensor:
		json_obj = _apiSensor(channel()->buffer());
		break;
	}
	json_str = json_object_to_json_string(json_obj);
	if (json_str == NULL || strcmp(json_str, "null") == 0) {
		print(log_debug, "JSON request body is null. Nothing to send now.", channel()->name());
		json_object_put(json_obj); // TODO untested!
		return;
	}

	print(log_debug, "JSON request body: '%s'", channel()->name(), json_str);

	/* initialize response */
	_response->clear_response();

	curl_easy_setopt(_curlIF.handle(), CURLOPT_POSTFIELDS, json_str);

	_api_header();
	const char *secret = secretKey();
	hmac_sha1(digest, (const unsigned char *)json_str, strlen(json_str),
			  reinterpret_cast<const unsigned char *>(secret), strlen(secret));
	_curlIF.addHeader(digest);
	print(log_debug, "Header_Digest: %s", channel()->name(), digest);

	_curlIF.commitHeader();

	curl_code = _curlIF.perform();
	curl_easy_getinfo(_curlIF.handle(), CURLINFO_RESPONSE_CODE, &http_code);

	/* check response */
	if (curl_code == CURLE_OK && http_code == 200) { /* everything is ok */
		print(log_debug, "Request succeeded with code: %i", channel()->name(), http_code);
		_values.clear();
	} else { /* error */
		channel()->buffer()->undelete();
		if (curl_code != CURLE_OK) {
			print(log_alert, "CURL: %s", channel()->name(), curl_easy_strerror(curl_code));
		} else if (http_code != 200) {
			// 502 - Bad gateway
			char err[255];
			api_parse_exception(err, 255);
			print(log_alert, "Error from middleware: %s", channel()->name(), err);
		}
	}

	/* householding */
	json_object_put(json_obj);

	if ((curl_code != CURLE_OK || http_code != 200)) {
		print(log_info, "Waiting %i secs for next request due to previous failure",
			  channel()->name(), options.retry_pause());
		sleep(options.retry_pause());
	}
	// sleep(20);
}

void vz::api::MySmartGrid::register_device() {
	OptionList optlist;
	// print(log_debug, "Register device: '%s'", channel()->name(), channel()->uuid());

	char url[255];
	sprintf(url, "%s/device/%s", middleware().c_str(), _deviceId.c_str()); /* build url */

	json_object *json_obj;

	std::string sensorName;
	try {
		convertUuid(optlist.lookup_string(channel()->options(), "name"), sensorName);

		// register device
		json_obj = _json_object_registration();
		_send(url, json_obj);

		// send a heartbeat
		json_obj = _json_object_heartbeat();
		_send(url, json_obj);

		// register sensor
		sprintf(url, "%s/sensor/%s", middleware().c_str(), uuid()); /* build url */
		json_obj = _json_object_sensor(sensorName);
		_send(url, json_obj);

	} catch (vz::OptionNotFoundException &e) {
		throw;
	} catch (vz::VZException &e) {
		throw;
	}
}

void vz::api::MySmartGrid::_send(const std::string &url, json_object *json_obj) {
	char digest[255];

	const char *json_str;
	long int http_code;
	CURLcode curl_code;

	print(log_debug, "msg_api_send() %s", channel()->name(), url.c_str());
	curl_easy_setopt(_curlIF.handle(), CURLOPT_URL, url.c_str());

	json_str = json_object_to_json_string(json_obj);
	if (json_str == NULL || strcmp(json_str, "null") == 0) {
		print(log_debug, "JSON request body is null. Nothing to send now.", channel()->name());
		return;
	}

	print(log_debug, "JSON request body: '%s'", channel()->name(), json_str);

	/* initialize response */
	_response->clear_response();

	curl_easy_setopt(_curlIF.handle(), CURLOPT_POSTFIELDS, json_str);

	_api_header();
	const char *secret = secretKey();
	hmac_sha1(digest, (const unsigned char *)json_str, strlen(json_str),
			  reinterpret_cast<const unsigned char *>(secret), strlen(secret));

	_curlIF.addHeader(digest);
	print(log_debug, "Header_Digest: %s", channel()->name(), digest);

	_curlIF.commitHeader();

	curl_code = _curlIF.perform();
	curl_easy_getinfo(_curlIF.handle(), CURLINFO_RESPONSE_CODE, &http_code);

	/* check response */
	if (curl_code == CURLE_OK && http_code == 200) { /* everything is ok */
		print(log_debug, "Request succeeded with code: %i", channel()->name(), http_code);
		_values.clear();
	} else { /* error */
		channel()->buffer()->undelete();
		if (curl_code != CURLE_OK) {
			print(log_alert, "CURL: %s", channel()->name(), curl_easy_strerror(curl_code));
		} else if (http_code != 200) {
			// 502 - Bad gateway
			char err[255];
			api_parse_exception(err, 255);
			print(log_alert, "Error from middleware: %s", channel()->name(), err);
		}
	}

	/* householding */
	json_object_put(json_obj);

	if ((curl_code != CURLE_OK || http_code != 200)) {
		print(log_info, "Waiting %i secs for next request due to previous failure",
			  channel()->name(), options.retry_pause());
		sleep(options.retry_pause());
	}
}

void vz::api::MySmartGrid::api_parse_exception(char *err, size_t n) {
	struct json_tokener *json_tok;
	struct json_object *json_obj_in;

	json_tok = json_tokener_new();
	json_obj_in = json_tokener_parse_ex(json_tok, _response->get_response().c_str(),
										_response->get_response().size());

	if (json_tok->err == json_tokener_success) {
		struct json_object *json_obj;
		bool found = json_object_object_get_ex(json_obj_in, "exception", &json_obj);
		if (found && json_obj) {
			struct json_object *j2 = 0, *j3 = 0;
			(void)json_object_object_get_ex(json_obj, "type",
											&j2); // we rely on ..._get_string to handle null
			(void)json_object_object_get_ex(json_obj, "message",
											&j3); // we rely on ..._get_string to handle null
			snprintf(err, n, "%s: %s", json_object_get_string(j2), json_object_get_string(j3));
		} else {
			bool found = json_object_object_get_ex(json_obj_in, "response", &json_obj);
			if (found && json_obj) {
				snprintf(err, n, "Server-Error: %s", json_object_get_string(json_obj));
			} else {
				print(log_debug, "CURL - Resp: '%s'", channel()->name(),
					  _response->get_response().c_str());
				strncpy(err, "missing exception", n);
			}
		}
	} else {
		strncpy(err, json_tokener_error_desc(json_tok->err), n);
	}

	json_object_put(json_obj_in);
	json_tokener_free(json_tok);
}

json_object *vz::api::MySmartGrid::_apiDevice(Buffer::Ptr buf) {

	// copy all values to local buffer queue
	buf->lock();
	for (Buffer::iterator it = buf->begin(); it != buf->end(); it++) {
		it->mark_delete();
	}
	buf->unlock();
	buf->clean();

	if (_first_ts > 0) { // send lifesign
		_first_ts = time(NULL);
		return _json_object_heartbeat();
	} else { // send  device registration
		_first_ts = time(NULL);
		return _json_object_registration();
	}
}

json_object *vz::api::MySmartGrid::_apiSensor(Buffer::Ptr buf) {

	return _json_object_measurements(buf);
}

/*---------------------------------------------------------------------*/
/**
 * @brief MySmartGrid Device Registration *  @param[in] buf
 * @return json-object
 **/
/*---------------------------------------------------------------------*/
json_object *vz::api::MySmartGrid::_json_object_registration() {
	// url https://api.mysmartgrid.de:8443/device/<device id>
	// key : <device-id>
	json_object *json_obj = json_object_new_object();

	json_object *jstring = json_object_new_string(secretKey());
	json_object_object_add(json_obj, "key", jstring);

	return json_obj;
}

/*---------------------------------------------------------------------*/
/**
 * @brief MySmartGrid heartbeat message
 * @param[in] buf
 * @return <ReturnValue>
 **/
/*---------------------------------------------------------------------*/
json_object *vz::api::MySmartGrid::_json_object_heartbeat() {
	// url https://api.mysmartgrid.de:8443/device/<device id>
	//  memtotal:   <total RAM>,
	//  version:    <firmware version>,
	//  memcached:  <total cache memory in MB>,
	//  membuffers: <total buffer memory in MB>,
	//  memfree:    <total free memory in MB>,
	//  uptime:     <total time the device is up and running>,
	//  reset:      <number of times the device has been reseted>
	json_object *json_obj = json_object_new_object();

	json_object_object_add(json_obj, "memtotal", json_object_new_int(128));
	json_object_object_add(json_obj, "version", json_object_new_string("1.0.0"));
	json_object_object_add(json_obj, "memcached", json_object_new_int(128));
	json_object_object_add(json_obj, "membuffers", json_object_new_int(12));
	json_object_object_add(json_obj, "memfree", json_object_new_int(1));
	json_object_object_add(json_obj, "uptime", json_object_new_int(1));
	json_object_object_add(json_obj, "reset", json_object_new_int(1));

	return json_obj;
}

/*---------------------------------------------------------------------*/
/**
 * @brief MySmartGrid event message
 * @param[in] buf
 * @return <ReturnValue>
 **/
/*---------------------------------------------------------------------*/
json_object *vz::api::MySmartGrid::_json_object_event(Buffer::Ptr buf) {
	// url https://api.mysmartgrid.de:8443/event/<event id>
	// device: <device id>
	json_object *json_obj = json_object_new_object();

	json_object_object_add(json_obj, "device", json_object_new_string(channel()->uuid()));

	return json_obj;
}

/*---------------------------------------------------------------------*/
/**
 * @brief MySmartGrid sensor configuration
 * @param[in] buf
 * @return <ReturnValue>
 **/
/*---------------------------------------------------------------------*/
json_object *vz::api::MySmartGrid::_json_object_sensor(const std::string &sensorName) {
	// url https://api.mysmartgrid.de:8443/sensor/<sensor id>
	// device:    <device id>,
	// function:  <sensor name>
	json_object *json_obj_ext = json_object_new_object();
	json_object *json_obj = json_object_new_object();

	json_object_object_add(json_obj, "device", json_object_new_string(_deviceId.c_str()));
	json_object_object_add(json_obj, "function", json_object_new_string(sensorName.c_str()));
	//	json_object_object_add(json_obj, "class",    json_object_new_string("analog"));
	//	json_object_object_add(json_obj, "type",     json_object_new_string("electricity"));
	//	json_object_object_add(json_obj, "voltage",  json_object_new_int(230));
	//	json_object_object_add(json_obj, "current",  json_object_new_int(0));
	//	json_object_object_add(json_obj, "constant", json_object_new_int(0));
	json_object_object_add(json_obj, "enable", json_object_new_int(1));
	//	json_object_object_add(json_obj, "phase",   json_object_new_int(1));

	json_object_object_add(json_obj_ext, "config", json_obj);

	return json_obj_ext;
}

/*---------------------------------------------------------------------*/
/**
 * @brief MySmartGrid sensor measurements message
 * @param[in] buf
 * @return <ReturnValue>
 **/
/*---------------------------------------------------------------------*/
json_object *vz::api::MySmartGrid::_json_object_measurements(Buffer::Ptr buf) {
	//  measurements: [[<timestamp1>,<value1>], [<timestamp2>,<value2>], ... ,[<timestamp n>,<value
	//  n>]]
	json_object *json_obj = json_object_new_object();
	json_object *json_tuples = json_object_new_array();
	Buffer::iterator it;

	// long last_counter = 0;

	long timestamp = 0;
	long value = 0.0;

	// print(log_debug, "MSG-API, buffer has %d element.", channel()->name(), buf->size());
	if (_values.size()) {
		timestamp = _values.back().time_s();
		value = _values.back().value();
	}

	// copy all values to local buffer queue
	buf->lock();
	for (it = buf->begin(); it != buf->end(); it++) {
		if (timestamp < it->time_s() /*&& value != (long)(it->value() * _scaler)*/) {
			_values.push_back(*it);
			timestamp = it->time_s();
			value = it->value() * _scaler;
		}
		it->mark_delete();
	}
	buf->unlock();
	buf->clean();

	// print(log_debug, "Valuescounter: %d", channel()->name(), _values.size());

	for (it = _values.begin(); it != _values.end(); it++) {
		timestamp = it->time_s();
		value = it->value() * _scaler;
		print(log_debug, "==> %ld, %lf - %ld", channel()->name(), timestamp, it->value(), value);
	}
	if (_values.size() < 1 || (_values.size() < 2 && _first_counter == 0)) {
		return NULL;
	}

	for (it = _values.begin(); it != _values.end(); it++) {
		struct json_object *json_tuple = json_object_new_array();

		// TODO use long int of new json-c version
		// API requires milliseconds => * 1000
		long timestamp = it->time_s();
		long value = it->value() * _scaler;

		if (_first_counter < 1) {
			_first_counter = value;
			_last_counter = value;
		} else {
			if (/*(_last_counter < value)  &&*/ (_first_ts < timestamp)) {
				_first_ts = timestamp;
				json_object_array_add(json_tuple, json_object_new_int(timestamp));
				json_object_array_add(json_tuple, json_object_new_int(value - _first_counter));
				json_object_array_add(json_tuples, json_tuple);
				_last_counter = value;
			} // else return NULL;
		}
	}

	json_object_object_add(json_obj, "measurements", json_tuples);

	return json_obj;
}

void vz::api::MySmartGrid::_api_header() {
	char agent[255];

	/* prepare header */
	sprintf(agent, "User-Agent: %s/%s (%s)", PACKAGE, VERSION,
			curl_version()); /* build user agent */

	_curlIF.clearHeader();

	_curlIF.addHeader("Content-type: application/json");
	//_curlIF.addHeader("Accept: application/json");
	_curlIF.addHeader(agent);
	_curlIF.addHeader("X-Version: 1.0");
}

void vz::api::MySmartGrid::convertUuid(const std::string uuidIn, std::string &uuidOut) {
	std::stringstream oss;
	for (size_t i = 0; i < uuidIn.length(); i++) {
		if (uuidIn[i] != '-')
			oss << uuidIn[i];
	}
	uuidOut = oss.str();
}

void vz::api::MySmartGrid::convertUuid(const std::string uuid) {
	std::stringstream oss;
	for (size_t i = 0; i < uuid.length(); i++) {
		if (uuid[i] != '-')
			oss << uuid[i];
	}
	_uuid = oss.str();
}
