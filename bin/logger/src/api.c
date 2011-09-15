/**
 * Implementation of volkszaehler.org API calls
 *
 * @author Steffen Vogel <info@steffenvogel.de>
 * @copyright Copyright (c) 2011, The volkszaehler.org project
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <reading.h>

#include "api.h"
#include "vzlogger.h"
#include "options.h"

extern options_t opts;

/**
 * Reformat CURLs debugging output
 */
int curl_custom_debug_callback(CURL *curl, curl_infotype type, char *data, size_t size, void *arg) {
	channel_t *ch = (channel_t *) ch;
	char *end = strchr(data, '\n');

	if (data == end) return 0; /* skip empty line */

	switch (type) {
		case CURLINFO_TEXT:
		case CURLINFO_END:
			if (end) *end = '\0'; /* terminate without \n */
			print(7, "CURL: %.*s", ch, (int) size, data);
			break;

		case CURLINFO_SSL_DATA_IN:
		case CURLINFO_DATA_IN:
			print(9, "CURL: Received %lu bytes", ch, (unsigned long) size);
			break;

		case CURLINFO_SSL_DATA_OUT:
		case CURLINFO_DATA_OUT:
			print(9, "CURL: Sent %lu bytes.. ", ch, (unsigned long) size);
			break;

		case CURLINFO_HEADER_IN:
		case CURLINFO_HEADER_OUT:
			break;
	}

	return 0;
}

size_t curl_custom_write_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	CURLresponse *response = (CURLresponse *) data;

	response->data = realloc(response->data, response->size + realsize + 1);
	if (response->data == NULL) { /* out of memory! */
		print(-1, "Not enough memory", NULL);
		exit(EXIT_FAILURE);
	}

	memcpy(&(response->data[response->size]), ptr, realsize);
	response->size += realsize;
	response->data[response->size] = 0;

	return realsize;
}

double api_tvtof(struct timeval tv) {
	return round(tv.tv_sec * 1000.0  + tv.tv_usec / 1000.0);
}

/**
 * Create JSON object of tuples
 *
 * @param buf	the buffer our readings are stored in (required for mutex)
 * @param first	the first tuple of our linked list which should be encoded
 * @param last	the last tuple of our linked list which should be encoded
 */
json_object * api_json_tuples(buffer_t *buf, meter_reading_t *first, meter_reading_t *last) {
	json_object *json_tuples = json_object_new_array();
	meter_reading_t *it;

	for (it = first; it != last->next; it = it->next) {
		struct json_object *json_tuple = json_object_new_array();

		pthread_mutex_lock(&buf->mutex);
		double timestamp = api_tvtof(it->tv); // TODO use long int of new json-c version
		double value = it->value;
		pthread_mutex_unlock(&buf->mutex);

		json_object_array_add(json_tuple, json_object_new_double(timestamp));
		json_object_array_add(json_tuple, json_object_new_double(value));

		json_object_array_add(json_tuples, json_tuple);
	}

	return json_tuples;
}

CURL * api_curl_init(channel_t *ch) {
	CURL *curl;
	struct curl_slist *header = NULL;
	char url[255], agent[255];

	/* prepare header, uuid & url */
	sprintf(agent, "User-Agent: %s/%s (%s)", PACKAGE, VERSION, curl_version());	/* build user agent */
	sprintf(url, "%s/data/%s.json", ch->middleware, ch->uuid);			/* build url */

	header = curl_slist_append(header, "Content-type: application/json");
	header = curl_slist_append(header, "Accept: application/json");
	header = curl_slist_append(header, agent);

	curl = curl_easy_init();
	if (!curl) {
		print(-1, "CURL: cannot create handle", ch);
		exit(EXIT_FAILURE);
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, (int) opts.verbose);
	curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curl_custom_debug_callback);
	curl_easy_setopt(curl, CURLOPT_DEBUGDATA, (void *) ch);

	return curl;
}

void api_parse_exception(CURLresponse response, char *err) {
	struct json_tokener *json_tok;
	struct json_object *json_obj;

	json_tok = json_tokener_new();
	json_obj = json_tokener_parse_ex(json_tok, response.data, response.size);
	if (json_tok->err == json_tokener_success) {
		json_obj = json_object_object_get(json_obj, "exception");

		if (json_obj) {
			sprintf(err, "%s: %s",
				json_object_get_string(json_object_object_get(json_obj,  "type")),
				json_object_get_string(json_object_object_get(json_obj,  "message"))
			);
		}
		else {
			strcpy(err, "missing exception");
		}
	}
	else {
		strcpy(err, json_tokener_errors[json_tok->err]);
	}

	json_object_put(json_obj);
	json_tokener_free(json_tok);
}


/**
 * Logging thread
 *
 * Logs buffered readings against middleware
 */
void logging_thread_cleanup(void *arg) {
	curl_easy_cleanup((CURL *) arg); /* always cleanup */
}
 
void * logging_thread(void *arg) {
	CURL *curl;
	channel_t *ch = (channel_t *) arg; /* casting argument */
	
	/* we don't to log in local only mode */
	if (opts.local && !opts.daemon) {
		return NULL;
	}

	curl = api_curl_init(ch);
	pthread_cleanup_push(&logging_thread_cleanup, curl);

	do { /* start thread mainloop */
		CURLresponse response;
		json_object *json_obj;
		meter_reading_t *last;

		const char *json_str;
		long int http_code, curl_code;

		/* initialize response */
		response.data = NULL;
		response.size = 0;

		pthread_mutex_lock(&ch->buffer.mutex);
		while (ch->buffer.sent == NULL) { /* detect spurious wakeups */
			pthread_cond_wait(&ch->condition, &ch->buffer.mutex); /* sleep until new data has been read */
		}
		pthread_mutex_unlock(&ch->buffer.mutex);
		
		last = ch->buffer.last;
		json_obj = api_json_tuples(&ch->buffer, ch->buffer.sent, last);
		json_str = json_object_to_json_string(json_obj);
		
		print(10, "JSON request body: %s", ch, json_str);

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_custom_write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &response);

		curl_code = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

		/* check response */
		if (curl_code == CURLE_OK && http_code == 200) { /* everything is ok */
			print(4, "Request succeeded with code: %i", ch, http_code);
			ch->buffer.sent = last->next;
		}
		else { /* error */
			if (curl_code != CURLE_OK) {
				print(-1, "CURL: %s", ch, curl_easy_strerror(curl_code));
			}
			else if (http_code != 200) {
				char err[255];
				api_parse_exception(response, err);
				print(-1, "Error from middleware: %s", ch, err);
			}
		}
		
		/* householding */
		free(response.data);
		json_object_put(json_obj);
		
		if (opts.daemon && (curl_code != CURLE_OK || http_code != 200)) {
			print(2, "Sleeping %i seconds due to previous failure", ch, RETRY_PAUSE);
			sleep(RETRY_PAUSE);
		}
	} while (opts.daemon);
	
	pthread_cleanup_pop(1);

	return NULL;
}
