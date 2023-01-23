/***********************************************************************/
/** @file Volkszaehler.cpp
 * Header file for volkszaehler.org API calls
 *
 * @author Steffen Vogel <info@steffenvogel.de>
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
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

#include <curl/curl.h>
#include <json-c/json.h>
#include <math.h>
#include <stddef.h>
#include <sys/time.h>
#include <unistd.h>

#include "Config_Options.hpp"
#include "CurlSessionProvider.hpp"
#include <VZException.hpp>
#include <api/Volkszaehler.hpp>

extern Config_Options options;

const int MAX_CHUNK_SIZE = 64;

vz::api::Volkszaehler::Volkszaehler(Channel::Ptr ch, std::list<Option> pOptions)
	: ApiIF(ch), _last_timestamp(0), _lastReadingSent(0) {
	OptionList optlist;
	char agent[255];

	// parse options
	try {
		_middleware = optlist.lookup_string(pOptions, "middleware");
	} catch (vz::OptionNotFoundException &e) {
		print(log_alert, "api volkszaehler requires parameter \"middleware\" but it's missing!",
			  ch->name());
		throw;
	} catch (vz::VZException &e) {
		print(log_alert,
			  "api volkszaehler requires parameter \"middleware\" as string but seems to have "
			  "different type!",
			  ch->name());
		throw;
	}

	try {
		_curlTimeout = optlist.lookup_int(pOptions, "timeout");
	} catch (vz::OptionNotFoundException &e) {
		_curlTimeout = 30; // 30 seconds default
	} catch (vz::VZException &e) {
		print(log_alert,
			  "api volkszaehler requires parameter \"timeout\" as integer but seems to have "
			  "different type!",
			  ch->name());
		throw;
	}

	// prepare header, uuid & url
	sprintf(agent, "User-Agent: %s/%s (%s)", PACKAGE, VERSION, curl_version()); // build user agent
	_url = _middleware;
	_url.append("/data/");
	_url.append(channel()->uuid());
	_url.append(".json");

	_api.headers = NULL;
	_api.headers = curl_slist_append(_api.headers, "Content-type: application/json");
	_api.headers = curl_slist_append(_api.headers, "Accept: application/json");
	_api.headers = curl_slist_append(_api.headers, agent);
}

vz::api::Volkszaehler::~Volkszaehler() {
	if (_lastReadingSent)
		delete _lastReadingSent;
}

void vz::api::Volkszaehler::send() {
	CURLresponse response;
	json_object *json_obj;

	const char *json_str;
	long int http_code;
	CURLcode curl_code;

	// initialize response
	response.data = NULL;
	response.size = 0;

	json_obj = api_json_tuples(channel()->buffer());
	json_str = json_object_to_json_string(json_obj);
	if (json_str == NULL || strcmp(json_str, "null") == 0) {
		print(log_debug, "JSON request body is null. Nothing to send now.", channel()->name());
		return;
	}

	_api.curl = curlSessionProvider
					? curlSessionProvider->get_easy_session(_middleware)
					: 0; // TODO add option to use parallel sessions. Simply add uuid() to the key.
	if (!_api.curl) {
		throw vz::VZException("CURL: cannot create handle.");
	}
	curl_easy_setopt(_api.curl, CURLOPT_URL, _url.c_str());
	curl_easy_setopt(_api.curl, CURLOPT_HTTPHEADER, _api.headers);
	curl_easy_setopt(_api.curl, CURLOPT_VERBOSE, options.verbosity());
	curl_easy_setopt(_api.curl, CURLOPT_DEBUGFUNCTION, curl_custom_debug_callback);
	curl_easy_setopt(_api.curl, CURLOPT_DEBUGDATA, channel().get());

	// signal-handling in libcurl is NOT thread-safe. so force to deactivated them!
	curl_easy_setopt(_api.curl, CURLOPT_NOSIGNAL, 1);

	// set timeout to 5 sec. required if next router has an ip-change.
	curl_easy_setopt(_api.curl, CURLOPT_TIMEOUT, _curlTimeout);

	print(log_debug, "JSON request body: %s", channel()->name(), json_str);

	curl_easy_setopt(_api.curl, CURLOPT_POSTFIELDS, json_str);
	curl_easy_setopt(_api.curl, CURLOPT_WRITEFUNCTION, curl_custom_write_callback);
	curl_easy_setopt(_api.curl, CURLOPT_WRITEDATA, (void *)&response);

	curl_code = curl_easy_perform(_api.curl);
	curl_easy_getinfo(_api.curl, CURLINFO_RESPONSE_CODE, &http_code);

	if (curlSessionProvider)
		curlSessionProvider->return_session(_middleware, _api.curl);

	// check response
	if (curl_code == CURLE_OK && http_code == 200) { // everything is ok
		print(log_debug, "CURL Request succeeded with code: %i", channel()->name(), http_code);
		if (_values.size() <= (size_t)MAX_CHUNK_SIZE) {
			print(log_finest, "emptied all (%d) values", channel()->name(), _values.size());
			_values.clear();
		} else {
			// remove only the first MAX_CHUNK_SIZE values:
			for (int i = 0; i < MAX_CHUNK_SIZE; ++i)
				_values.pop_front();
			print(log_finest, "emptied MAX_CHUNK_SIZE values", channel()->name());
		}
		// clear buffer-readings
		// channel()->buffer.sent = last->next;
	} else { // error
		if (curl_code != CURLE_OK) {
			print(log_alert, "CURL: %s", channel()->name(), curl_easy_strerror(curl_code));
		} else if (http_code != 200) {
			char err[255];
			api_parse_exception(response, err, 255);
			print(log_alert, "CURL Error from middleware: %s", channel()->name(), err);
		}
	}

	// householding
	free(response.data);
	json_object_put(json_obj);

	if ((curl_code != CURLE_OK || http_code != 200)) {
		print(log_info, "Waiting %i secs for next request due to previous failure",
			  channel()->name(), options.retry_pause());
		sleep(options.retry_pause());
	}
}

void vz::api::Volkszaehler::register_device() {}

json_object *vz::api::Volkszaehler::api_json_tuples(Buffer::Ptr buf) {

	Buffer::iterator it;

	print(log_debug, "==> number of tuples: %d", channel()->name(), buf->size());
	int64_t timestamp = 1;
	const int duplicates = channel()->duplicates();
	const int duplicates_ms = duplicates * 1000;

	// copy all values to local buffer queue
	buf->lock();
	for (it = buf->begin(); it != buf->end(); it++) {
		timestamp = it->time_ms();
		print(log_debug, "compare: %lld %lld", channel()->name(), _last_timestamp, timestamp);
		// we can only add/consider a timestamp if the ms resolution is different than from previous
		// one:
		if (_last_timestamp < timestamp) {
			if (0 == duplicates) { // send all values
				_values.push_back(*it);
				_last_timestamp = timestamp;
			} else {
				const Reading &r = *it;
				// duplicates should be ignored
				// but send at least each <duplicates> seconds

				if (!_lastReadingSent) { // first one from the duplicate consideration -> send it
					_lastReadingSent = new Reading(r);
					_values.push_back(r);
					_last_timestamp = timestamp;
				} else { // one reading sent already. compare
					// a) timestamp
					// b) duplicate value
					if ((timestamp >= (_last_timestamp + duplicates_ms)) ||
						(r.value() != _lastReadingSent->value())) {
						// send the current one:
						_values.push_back(r);
						_last_timestamp = timestamp;
						*_lastReadingSent = r;
					} else {
						// ignore it
					}
				}
			}
		}
		it->mark_delete();
	}
	buf->unlock();
	buf->clean();

	if (_values.size() < 1) {
		return NULL;
	}

	json_object *json_tuples = json_object_new_array();
	int nrTuples = 0;
	for (it = _values.begin(); it != _values.end(); it++) {
		struct json_object *json_tuple = json_object_new_array();

		json_object_array_add(json_tuple, json_object_new_int64(it->time_ms()));
		json_object_array_add(json_tuple, json_object_new_double(it->value()));

		json_object_array_add(json_tuples, json_tuple);
		++nrTuples;
		if (nrTuples >= MAX_CHUNK_SIZE)
			break;
	}
	print(log_finest, "copied %d/%d values for middleware transmission", channel()->name(),
		  nrTuples, _values.size());

	return json_tuples;
}

void vz::api::Volkszaehler::api_parse_exception(CURLresponse response, char *err, size_t n) {
	struct json_tokener *json_tok;
	struct json_object *json_obj;

	json_tok = json_tokener_new();
	json_obj = json_tokener_parse_ex(json_tok, response.data, response.size);

	if (json_tok->err == json_tokener_success) {
		bool found = json_object_object_get_ex(json_obj, "exception", &json_obj);
		if (found && json_obj) {
			struct json_object *j2;
			std::string err_type;
			if (json_object_object_get_ex(json_obj, "type", &j2)) {
				err_type = json_object_get_string(j2);
			}
			std::string err_message;
			if (json_object_object_get_ex(json_obj, "message", &j2)) {
				err_message = json_object_get_string(j2);
			}
			snprintf(err, n, "'%s': '%s'", err_type.c_str(), err_message.c_str());
			// evaluate error
			if (err_type == "UniqueConstraintViolationException") {
				if (err_message.find("Duplicate entry")) {
					print(log_warning, "Middleware says duplicated value. Removing first entry!",
						  channel()->name());
					_values.pop_front();
				}
			}
		} else {
			strncpy(err, "Missing exception", n);
		}
	} else {
		strncpy(err, json_tokener_error_desc(json_tok->err), n);
	}

	json_object_put(json_obj);
	json_tokener_free(json_tok);
}

int vz::api::curl_custom_debug_callback(CURL *curl, curl_infotype type, char *data, size_t size,
										void *arg) {
	Channel *ch = static_cast<Channel *>(arg);
	char *end = (char *)memchr(data, '\n', size);

	if (data == end)
		return 0; // skip empty line

	switch (type) {
	case CURLINFO_TEXT:
	case CURLINFO_END:
		print((log_level_t)(log_debug + 5), "CURL: %.*s", ch->name(),
			  (int)(end ? (end - data) : size), data);
		break;

	case CURLINFO_SSL_DATA_IN:
		print((log_level_t)(log_debug + 5), "CURL: Received %lu bytes SSL data", ch->name(),
			  (unsigned long)size);
		break;
	case CURLINFO_DATA_IN:
		print((log_level_t)(log_debug + 5), "CURL: Received %lu bytes: '%.*s'", ch->name(),
			  (unsigned long)size, (int)size, data);
		break;

	case CURLINFO_SSL_DATA_OUT:
		print((log_level_t)(log_debug + 5), "CURL: Sent %lu bytes SSL data", ch->name(),
			  (unsigned long)size);
		break;
	case CURLINFO_DATA_OUT:
		print((log_level_t)(log_debug + 5), "CURL: Sent %lu bytes: '%.*s'", ch->name(),
			  (unsigned long)size, (int)size, data);
		break;

	case CURLINFO_HEADER_IN:
	case CURLINFO_HEADER_OUT:
		break;
	}

	return 0;
}

size_t vz::api::curl_custom_write_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	CURLresponse *response = static_cast<CURLresponse *>(data);

	response->data = (char *)realloc(response->data, response->size + realsize + 1);
	if (response->data == NULL) { // out of memory!
		print(log_alert, "Cannot allocate memory", NULL);
		exit(EXIT_FAILURE);
	}

	memcpy(&(response->data[response->size]), ptr, realsize);
	response->size += realsize;
	response->data[response->size] = 0;

	return realsize;
}
