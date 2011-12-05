/**
 * Thread implementations
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
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
#include <unistd.h>

#include "threads.h"
#include "api.h"
#include "vzlogger.h"

extern config_options_t options;

void reading_thread_cleanup(void *rds) {
	free(rds);
}

void * reading_thread(void *arg) {
	reading_t *rds;
	map_t *mapping;
	meter_t *mtr;
	time_t last, delta;
	const meter_details_t *details;
	size_t n = 0;

	mapping = (map_t *) arg;
	mtr = &mapping->meter;
	details = meter_get_details(mtr->protocol);

	/* allocate memory for readings */
	size_t bytes = sizeof(reading_t) * details->max_readings;
	rds = malloc(bytes);
	memset(rds, 0, bytes);

	pthread_cleanup_push(&reading_thread_cleanup, rds);

	do { /* start thread main loop */
		/* fetch readings from meter and measure interval */
		last = time(NULL);
		n = meter_read(mtr, rds, details->max_readings);
		delta = time(NULL) - last;

		/* dumping meter output */
		if (options.verbosity > log_debug) {
			print(log_debug, "Got %i new readings from meter:", mtr, n);
			char identifier[255]; // TODO choose correct length
			for (int i = 0; i < n; i++) {
				switch (mtr->protocol) {
					case meter_protocol_d0:
					case meter_protocol_sml:
						obis_unparse(rds[i].identifier.obis, identifier, 255);
						break;

					case meter_protocol_file:
					case meter_protocol_exec:
						if (rds[i].identifier.string != NULL) {
							strncpy(identifier, rds[i].identifier.string, 255);
							break;
						}

					default:
						identifier[0] = '\0';
				}

				print(log_debug, "Reading: id=%s value=%.2f ts=%.3f", mtr, identifier, rds[i].value, tvtod(rds[i].time));
			}
		}

		/* update buffer length with current interval */
		if (!details->periodic && mtr->interval != delta) {
			print(log_debug, "Updating interval to %i", mtr, delta);
			mtr->interval = delta;
		}

		foreach(mapping->channels, ch, channel_t) {
			buffer_t *buf = &ch->buffer;
			reading_t *added = channel_add_readings(ch, mtr->protocol, rds, n);

			/* update buffer length to interval */
			if (options.local) {
				buf->keep = ceil(options.buffer_length / mtr->interval);
			}

			/* queue reading into sending buffer logging thread if
			   logging is enabled & sent queue is empty */
			if (options.logging && buf->sent == NULL) {
				buf->sent = added;
			}

			/* shrink buffer */
			buffer_clean(buf);

			/* notify webserver and logging thread */
			pthread_mutex_lock(&buf->mutex);
			pthread_cond_broadcast(&ch->condition);
			pthread_mutex_unlock(&buf->mutex);

			/* debugging */
			if (options.verbosity >= log_debug) {
				char dump[1024];
				buffer_dump(buf, dump, 1024);
				print(log_debug, "Buffer dump: %s (size=%i, keep=%i)", ch, dump, buf->size, buf->keep);
			}
		}

		if ((options.daemon || options.local) && details->periodic) {
			print(log_info, "Next reading in %i seconds", mtr, mtr->interval);
			sleep(mtr->interval);
		}
	} while (options.daemon || options.local);

	pthread_cleanup_pop(1);

	return NULL;
}

void logging_thread_cleanup(void *arg) {
	curl_easy_cleanup((CURL *) arg); /* always cleanup */
}

void * logging_thread(void *arg) {
	channel_t *ch = (channel_t *) arg; /* casting argument */
	CURL *curl = api_curl_init(ch);
	
	pthread_cleanup_push(&logging_thread_cleanup, curl);

	do { /* start thread mainloop */
		CURLresponse response;
		json_object *json_obj;

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

		reading_t *first = ch->buffer.sent;
		reading_t *last = ch->buffer.tail;
		json_obj = api_json_tuples(&ch->buffer, first, last);
		json_str = json_object_to_json_string(json_obj);

		print(log_debug, "JSON request body: %s", ch, json_str);

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_custom_write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &response);

		curl_code = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

		/* check response */
		if (curl_code == CURLE_OK && http_code == 200) { /* everything is ok */
			print(log_debug, "Request succeeded with code: %i", ch, http_code);
			ch->buffer.sent = last->next;
		}
		else { /* error */
			if (curl_code != CURLE_OK) {
				print(log_error, "CURL: %s", ch, curl_easy_strerror(curl_code));
			}
			else if (http_code != 200) {
				char err[255];
				api_parse_exception(response, err, 255);
				print(log_error, "Error from middleware: %s", ch, err);
			}
		}

		/* householding */
		free(response.data);
		json_object_put(json_obj);

		if (options.daemon && (curl_code != CURLE_OK || http_code != 200)) {
			print(log_info, "Waiting %i secs for next request due to previous failure", ch, options.retry_pause);
			sleep(options.retry_pause);
		}
	} while (options.daemon);

	pthread_cleanup_pop(1);

	return NULL;
}
