/**
 * Implementation of local interface via libmicrohttpd
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

#include <json/json.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "vzlogger.h"
#include "local.h"
#include "options.h"
#include "api.h"

extern list_t chans;
extern options_t opts;

int handle_request(void *cls, struct MHD_Connection *connection, const char *url, const char *method,
			const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) {

	int ret;
	const char *json_str;
	const char *uuid = url+1;

	struct timespec ts;
	struct timeval tp;

	struct MHD_Response *response;

	struct json_object *json_obj = json_object_new_object();
	struct json_object *json_data = json_object_new_object();

	print(2, "Local request received: %s %s %s", NULL, version, method, url);

	for (channel_t *ch = chans.start; ch != NULL; ch = ch->next) {
		if (strcmp(url, "/") == 0 || strcmp(ch->uuid, uuid) == 0) {
			/* convert from timeval to timespec */
			gettimeofday(&tp, NULL);
			ts.tv_sec  = tp.tv_sec;
			ts.tv_nsec = tp.tv_usec * 1000;
			ts.tv_sec += COMET_TIMEOUT;

			/* blocking until new data arrives (comet-like blocking of HTTP response) */
			pthread_mutex_lock(&ch->buffer.mutex);
			pthread_cond_timedwait(&ch->condition, &ch->buffer.mutex, &ts);
			pthread_mutex_unlock(&ch->buffer.mutex);

			json_object_object_add(json_data, "uuid", json_object_new_string(ch->uuid));
			json_object_object_add(json_data, "interval", json_object_new_int(ch->interval));

			struct json_object *json_tuples = api_json_tuples(&ch->buffer, ch->buffer.start);
			json_object_object_add(json_data, "tuples", json_tuples);
		}
	}

	json_object_object_add(json_obj, "version", json_object_new_string(VERSION));
	json_object_object_add(json_obj, "generator", json_object_new_string(PACKAGE));
	json_object_object_add(json_obj, "data", json_data);
	json_str = json_object_to_json_string(json_obj);

	response = MHD_create_response_from_data(strlen(json_str), (void *) json_str, FALSE, TRUE);

	MHD_add_response_header(response, "Content-type", "application/json");

	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);

	MHD_destroy_response (response);

	return ret;
}
