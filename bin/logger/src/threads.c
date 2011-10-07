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
#include "options.h"

extern options_t options;

void reading_thread_cleanup(void *rds) {
	free(rds);
}

void * reading_thread(void *arg) {
	assoc_t *assoc = (assoc_t *) arg;
	meter_t *mtr = &assoc->meter;
	reading_t *rds = malloc(sizeof(reading_t) * mtr->type->max_readings);
	time_t last, delta;
	size_t n = 0;
	
	pthread_cleanup_push(&reading_thread_cleanup, rds);
	
	do { /* start thread main loop */
		/* fetch readings from meter */
		
		last = time(NULL);
		n = meter_read(mtr, rds, mtr->type->max_readings);
		delta = time(NULL) - last;
		
		foreach(assoc->channels, it) {
			channel_t *ch = (channel_t *) it->data;
			buffer_t *buf = &ch->buffer;
			reading_t *added;
			
			for (int i = 0; i < n; i++) {
				switch (mtr->type->id) {
					case SML:
					case D0:
						if (obis_compare(rds[i].identifier.obis, ch->identifier.obis) == 0) {
							print(5, "New reading (value=%.2f ts=%f)", ch, ch->id, rds[i].value, tvtod(rds[i].time));
							added = buffer_push(buf, &rds[i]);
						}
						break;
						
					default:
						/* no channel identifier, adding all readings to buffer */
						print(5, "New reading (value=%.2f ts=%f)", ch, ch->id, rds[i].value, tvtod(rds[i].time));
						added = buffer_push(buf, &rds[i]);
				}
			}
			
			/* update buffer length with current interval */
			if (!mtr->type->periodic && options.local && delta) {
				print(15, "Updating interval to %i", ch, delta);
				ch->interval = delta;
				ch->buffer.keep = ceil(BUFFER_KEEP / (double) delta);
			}
		
			/* queue reading into sending buffer logging thread if
			   logging is enabled & sent queue is empty */
			if (options.logging && !buf->sent) {
				buf->sent = added;
			}
		
			/* shrink buffer */
			buffer_clean(buf);
		
			/* notify webserver and logging thread */
			pthread_mutex_lock(&buf->mutex);
			pthread_cond_broadcast(&ch->condition);
			pthread_mutex_unlock(&buf->mutex);
			
			/* debugging */
			if (options.verbose >= 10) {
				char dump[1024];
				buffer_dump(buf, dump, 1024);
				print(10, "Buffer dump: %s (size=%i, memory=%i)", ch, dump, buf->size, buf->keep);
			}
		}

		if ((options.daemon || options.local) && mtr->type->periodic) {
			print(8, "Next reading in %i seconds", NULL, 5);
			sleep(5); // TODO handle parsing
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
		reading_t *last;

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
		
		last = ch->buffer.tail;
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
				api_parse_exception(response, err, 255);
				print(-1, "Error from middleware: %s", ch, err);
			}
		}
		
		/* householding */
		free(response.data);
		json_object_put(json_obj);
		
		if (options.daemon && (curl_code != CURLE_OK || http_code != 200)) {
			print(2, "Sleeping %i seconds due to previous failure", ch, RETRY_PAUSE);
			sleep(RETRY_PAUSE);
		}
	} while (options.daemon);
	
	pthread_cleanup_pop(1);

	return NULL;
}
