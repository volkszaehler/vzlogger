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
#include "Channel.hpp"
#include "local.h"
#include <MeterMap.hpp>
#include <VZException.hpp>

extern Config_Options options;

int handle_request(
	void *cls
	, struct MHD_Connection *connection
	, const char *url
	, const char *method
	, const char *version
	, const char *upload_data
	, size_t *upload_data_size
	, void **con_cls
	) {

	int status;
	int response_code = MHD_HTTP_NOT_FOUND;

	/* mapping between meters and channels */
//std::list<Map> *mappings = static_cast<std::list<Map>*>(cls);
	MapContainer *mappings = static_cast<MapContainer*>(cls);

	struct MHD_Response *response;
	const char *mode = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "mode");

	try {
		print(log_info, "Local request received: method=%s url=%s mode=%s", 
					"http", method, url, mode);

		if (strcmp(method, "GET") == 0) {
			struct timespec ts;
			struct timeval tp;

			struct json_object *json_obj = json_object_new_object();
			struct json_object *json_data = json_object_new_array();
			struct json_object *json_exception = NULL;

			const char *uuid = url + 1; /* strip leading slash */
			const char *json_str;
			int show_all = 0;

			if (strcmp(url, "/") == 0) {
				if (options.channel_index()) {
					show_all = TRUE;
				}
				else {
					json_exception = json_object_new_object();

					json_object_object_add(json_exception, "message", json_object_new_string("channel index is disabled"));
					json_object_object_add(json_exception, "code", json_object_new_int(0));
				}
			}

			for(MapContainer::iterator mapping = mappings->begin(); mapping!=mappings->end(); mapping++) {
				for(MeterMap::iterator ch = mapping->begin(); ch!=mapping->end(); ch++) {
//foreach(mapping->channels, ch, channel_t) {
					if (strcmp((*ch)->uuid(), uuid) == 0 || show_all) {
						response_code = MHD_HTTP_OK;

/* blocking until new data arrives (comet-like blocking of HTTP response) */
						if (mode && strcmp(mode, "comet") == 0) {
/* convert from timeval to timespec */
							gettimeofday(&tp, NULL);
							ts.tv_sec  = tp.tv_sec + options.comet_timeout();
							ts.tv_nsec = tp.tv_usec * 1000;

							(*ch)->wait();
						}

						struct json_object *json_ch = json_object_new_object();

						json_object_object_add(json_ch, "uuid", json_object_new_string((*ch)->uuid()));
//json_object_object_add(json_ch, "middleware", json_object_new_string(ch->middleware()));
//json_object_object_add(json_ch, "last", json_object_new_double(ch->last.value));
						json_object_object_add(json_ch, "last", json_object_new_double((*ch)->tvtod()));
						json_object_object_add(json_ch, "interval", json_object_new_int(mapping->meter()->interval()));
						json_object_object_add(json_ch, "protocol", json_object_new_string(meter_get_details(mapping->meter()->protocolId())->name));

//struct json_object *json_tuples = api_json_tuples(&ch->buffer, ch->buffer.head, ch->buffer.tail);
//json_object_object_add(json_ch, "tuples", json_tuples);

						json_object_array_add(json_data, json_ch);
					}
				}
			}

			json_object_object_add(json_obj, "version", json_object_new_string(VERSION));
			json_object_object_add(json_obj, "generator", json_object_new_string(PACKAGE));
			json_object_object_add(json_obj, "data", json_data);

			if (json_exception) {
				json_object_object_add(json_obj, "exception", json_exception);
			}

			json_str = json_object_to_json_string(json_obj);
			response = MHD_create_response_from_data(strlen(json_str), (void *) json_str, FALSE, TRUE);
			json_object_put(json_obj);

			MHD_add_response_header(response, "Content-type", "application/json");
		}
		else {
			char *response_str = strdup("not implemented\n");

			response = MHD_create_response_from_data(strlen(response_str), (void *) response_str, TRUE, FALSE);
			response_code = MHD_HTTP_METHOD_NOT_ALLOWED;

			MHD_add_response_header(response, "Content-type", "text/text");
		}
	} catch ( std::exception &e){
	}

	status = MHD_queue_response(connection, response_code, response);

	MHD_destroy_response(response);

	return status;
}
