/**
 * Implementation of volkszaehler.org API calls
 *
 * @author Steffen Vogel <info@steffenvogel.de>
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
 * @package vzlogger
 * @license http://opensource.org/licenses/gpl-license.php GNU Public License
 */
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "api.h"
#include "meter.h"
#include "vzlogger.h"
#include <VZException.hpp>

extern Config_Options options;

int curl_custom_debug_callback(CURL *curl, curl_infotype type, char *data, size_t size, void *arg) {
	Channel *ch = static_cast<Channel *>(arg);
	char *end = strchr(data, '\n');

	if (data == end)
		return 0; /* skip empty line */

	switch (type) {
	case CURLINFO_TEXT:
	case CURLINFO_END:
		if (end)
			*end = '\0'; /* terminate without \n */
		print((log_level_t)(log_debug + 5), "CURL: %.*s", ch->name(), (int)size, data);
		break;

	case CURLINFO_SSL_DATA_IN:
	case CURLINFO_DATA_IN:
		print((log_level_t)(log_debug + 5), "CURL: Received %lu bytes", ch->name(),
			  (unsigned long)size);
		break;

	case CURLINFO_SSL_DATA_OUT:
	case CURLINFO_DATA_OUT:
		print((log_level_t)(log_debug + 5), "CURL: Sent %lu bytes.. ", ch->name(),
			  (unsigned long)size);
		break;

	case CURLINFO_HEADER_IN:
	case CURLINFO_HEADER_OUT:
		break;
	}

	return 0;
}

size_t curl_custom_write_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	CURLresponse *response = static_cast<CURLresponse *>(data);

	response->data = (char *)realloc(response->data, response->size + realsize + 1);
	if (response->data == NULL) { /* out of memory! */
		print(log_alert, "Cannot allocate memory", NULL);
		exit(EXIT_FAILURE);
	}

	memcpy(&(response->data[response->size]), ptr, realsize);
	response->size += realsize;
	response->data[response->size] = 0;

	return realsize;
}

json_object *api_json_tuples(Buffer::Ptr buf) {

	json_object *json_tuples = json_object_new_array();
	Buffer::iterator it;

	print(log_debug, "==> number of tuples: %d", "api", buf->size());

	for (it = buf->begin(); it != buf->end(); it++) {
		struct json_object *json_tuple = json_object_new_array();

		buf->lock();

		// TODO use long int of new json-c version
		// API requires milliseconds => * 1000
		double timestamp = it->tvtod() * 1000;
		double value = it->value();
		buf->unlock();

		json_object_array_add(json_tuple, json_object_new_double(timestamp));
		json_object_array_add(json_tuple, json_object_new_double(value));

		json_object_array_add(json_tuples, json_tuple);
	}

	return json_tuples;
}

vz::Api::Api(Channel::Ptr ch)
	: _ch(ch)
//    , _api(api)
{
	char url[255], agent[255];

	/* prepare header, uuid & url */
	sprintf(agent, "User-Agent: %s/%s (%s)", PACKAGE, VERSION,
			curl_version());                                         /* build user agent */
	sprintf(url, "%s/data/%s.json", _ch->middleware(), _ch->uuid()); /* build url */

	_api.headers = NULL;
	_api.headers = curl_slist_append(_api.headers, "Content-type: application/json");
	_api.headers = curl_slist_append(_api.headers, "Accept: application/json");
	_api.headers = curl_slist_append(_api.headers, agent);

	// WORKAROUND for lighthttp not supporting Expect: properly
	_api.headers = curl_slist_append(_api.headers, "Expect: ");

	_api.curl = curl_easy_init();
	if (!_api.curl) {
		throw vz::VZException("CURL: cannot create handle.");
	}

	curl_easy_setopt(_api.curl, CURLOPT_URL, url);
	curl_easy_setopt(_api.curl, CURLOPT_HTTPHEADER, _api.headers);
	curl_easy_setopt(_api.curl, CURLOPT_VERBOSE, options.verbosity());
	curl_easy_setopt(_api.curl, CURLOPT_DEBUGFUNCTION, curl_custom_debug_callback);
	curl_easy_setopt(_api.curl, CURLOPT_DEBUGDATA, _ch.get());
}

vz::Api::~Api() {
	curl_easy_cleanup(_api.curl);
	curl_slist_free_all(_api.headers);
}

void api_parse_exception(CURLresponse response, char *err, size_t n) {
	struct json_tokener *json_tok;
	struct json_object *json_obj;

	json_tok = json_tokener_new();
	json_obj = json_tokener_parse_ex(json_tok, response.data, response.size);

	if (json_tok->err == json_tokener_success) {
		json_obj = json_object_object_get(json_obj, "exception");

		if (json_obj) {
			snprintf(err, n, "%s: %s",
					 json_object_get_string(json_object_object_get(json_obj, "type")),
					 json_object_get_string(json_object_object_get(json_obj, "message")));
		} else {
			strncpy(err, "missing exception", n);
		}
	} else {
		strncpy(err, json_tokener_error_desc(json_tok->err), n);
	}

	json_object_put(json_obj);
	json_tokener_free(json_tok);
}
