/**
 * Implementation of local interface via libmicrohttpd
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
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

#include <list>
#include <map>

#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "Channel.hpp"
#include "local.h"
#include "vzlogger.h"
#include <MeterMap.hpp>
#include <VZException.hpp>
#include <pthread.h>

extern Config_Options options;

class ChannelData {
  public:
	ChannelData(const int64_t &t, const double &v) : _t(t), _v(v){};
	int64_t _t;
	double _v;
};

typedef std::list<ChannelData> LIST_ChannelData;
typedef std::map<std::string, LIST_ChannelData> MAP_UUID_ChannelData;
pthread_mutex_t localbuffer_mutex = PTHREAD_MUTEX_INITIALIZER;
MAP_UUID_ChannelData localbuffer;

void shrink_localbuffer() // remove old data in the local buffer
{
	if (options.buffer_length() >= 0) { // time based localbuffer. keep buffer_length secs
		Reading rnow;
		rnow.time(); // sets to "now"
		int64_t minT =
			rnow.time_ms() - (1000 * options.buffer_length()); // now - time to keep in buffer

		pthread_mutex_lock(&localbuffer_mutex);

		MAP_UUID_ChannelData::iterator it = localbuffer.begin();
		for (; it != localbuffer.end(); ++it) {
			LIST_ChannelData &l = it->second;
			LIST_ChannelData::iterator lit = l.begin();

			while (lit != l.end() && ((lit->_t) < minT))
				lit = l.erase(lit);
		}

		pthread_mutex_unlock(&localbuffer_mutex);
	}
}

void add_ch_to_localbuffer(Channel &ch) {
	pthread_mutex_lock(&localbuffer_mutex);
	LIST_ChannelData &l = localbuffer[ch.uuid()];

	// now add all not-deleted items to the localbuffer:
	Buffer::Ptr buf = ch.buffer();
	Buffer::iterator it;
	for (it = buf->begin(); it != buf->end(); ++it) {
		Reading &r = *it;
		if (!r.deleted()) {
			l.push_back(ChannelData(r.time_ms(), r.value()));
		}
	}
	if (options.buffer_length() < 0) { // max size based localbuffer. keep max -buffer_length items
		while (l.size() > static_cast<unsigned int>(-(options.buffer_length())))
			l.pop_front();
	}

	pthread_mutex_unlock(&localbuffer_mutex);
}

json_object *api_json_tuples(const char *uuid) {

	if (!uuid)
		return NULL;
	pthread_mutex_lock(&localbuffer_mutex);
	LIST_ChannelData &l = localbuffer[uuid];

	print(log_debug, "==> number of tuples: %d", uuid, l.size());

	if (l.size() < 1) {
		pthread_mutex_unlock(&localbuffer_mutex);
		return NULL;
	}

	json_object *json_tuples = json_object_new_array();
	for (LIST_ChannelData::const_iterator cit = l.cbegin(); cit != l.cend(); ++cit) {
		struct json_object *json_tuple = json_object_new_array();

		json_object_array_add(json_tuple, json_object_new_int64(cit->_t));
		json_object_array_add(json_tuple, json_object_new_double(cit->_v));

		json_object_array_add(json_tuples, json_tuple);
	}
	pthread_mutex_unlock(&localbuffer_mutex);

	return json_tuples;
}

MHD_RESULT handle_request(void *cls, struct MHD_Connection *connection, const char *url,
						  const char *method, const char *version, const char *upload_data,
						  size_t *upload_data_size, void **con_cls) {

	MHD_RESULT status;
	int response_code = MHD_HTTP_NOT_FOUND;

	// mapping between meters and channels
	MapContainer *mappings = static_cast<MapContainer *>(cls);

	struct MHD_Response *response = NULL;
	const char *mode = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "mode");

	try {
		print(log_info, "Local request received: method=%s url=%s mode=%s", "http", method, url,
			  mode);

		if (strcmp(method, "GET") == 0) {

			struct json_object *json_obj = json_object_new_object();
			struct json_object *json_data = json_object_new_array();
			struct json_object *json_exception = NULL;

			const char *uuid = url + 1; // strip leading slash
			const char *json_str;
			bool show_all = false;

			if (strcmp(url, "/") == 0) {
				if (options.channel_index()) {
					show_all = true;
				} else {
					json_exception = json_object_new_object();

					json_object_object_add(json_exception, "message",
										   json_object_new_string("channel index is disabled"));
					json_object_object_add(json_exception, "code", json_object_new_int(0));
				}
			}

			shrink_localbuffer(); // in case the channel return very few/seldom data

			for (MapContainer::iterator mapping = mappings->begin(); mapping != mappings->end();
				 mapping++) {
				for (MeterMap::iterator ch = mapping->begin(); ch != mapping->end(); ch++) {
					if (strcmp((*ch)->uuid(), uuid) == 0 || show_all) {
						response_code = MHD_HTTP_OK;

						// blocking until new data arrives (comet-like blocking of HTTP response)
						if (mode && strcmp(mode, "comet") == 0) {
							// TODO wait only options.comet_timeout()!
							// gettimeofday(&tp, NULL);
							// ts.tv_sec = tp.tv_sec + options.comet_timeout();
							// ts.tv_nsec = tp.tv_usec * 1000;

							// (*ch)->wait(); // TODO not usefull with
							// 			   // show_all! Wait only if this channel empty?
						}

						struct json_object *json_ch = json_object_new_object();

						json_object_object_add(json_ch, "uuid",
											   json_object_new_string((*ch)->uuid()));
						json_object_object_add(
							json_ch, "last",
							json_object_new_int64((*ch)->time_ms())); // return here in ms as well
						json_object_object_add(json_ch, "interval",
											   json_object_new_int(mapping->meter()->interval()));
						json_object_object_add(
							json_ch, "protocol",
							json_object_new_string(
								meter_get_details(mapping->meter()->protocolId())->name));

						struct json_object *json_tuples = api_json_tuples((*ch)->uuid());
						if (json_tuples)
							json_object_object_add(json_ch, "tuples", json_tuples);

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
			response = MHD_create_response_from_buffer(
				strlen(json_str), static_cast<void *>(const_cast<char *>(json_str)),
				MHD_RESPMEM_MUST_COPY);
			json_object_put(json_obj);

			MHD_add_response_header(response, "Content-type", "application/json");
		} else {
			char *response_str = strdup("not implemented\n");

			response = MHD_create_response_from_buffer(
				strlen(response_str), static_cast<void *>(const_cast<char *>(response_str)),
				MHD_RESPMEM_MUST_COPY);
			response_code = MHD_HTTP_METHOD_NOT_ALLOWED;

			MHD_add_response_header(response, "Content-type", "text/text");
		}
	} catch (std::exception &e) {
	}

	status = MHD_queue_response(connection, response_code, response);

	MHD_destroy_response(response);

	return status;
}
