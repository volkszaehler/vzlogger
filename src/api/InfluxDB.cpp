/***********************************************************************/
/** @file InfluxDB.cpp
 * Header file for InfluxDB API calls
 *
 * @author Stefan Kuntz
 * @email  Stefan.github@gmail.com
 * @copyright Copyright (c) 2017, The volkszaehler.org project
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

#include <stdio.h>
#include <curl/curl.h>
#include "Config_Options.hpp"
#include <api/InfluxDB.hpp>
#include <VZException.hpp>
#include <api/CurlCallback.hpp>
#include "CurlSessionProvider.hpp"
extern Config_Options options;

vz::api::InfluxDB::InfluxDB(
	Channel::Ptr ch,
	std::list<Option> pOptions
	)
	: ApiIF(ch)
{
	OptionList optlist;
	print(log_info, "InfluxDB API initialize", ch->name());

// config file options
try {
		_host = optlist.lookup_string(pOptions, "host");
		print(log_finest, "api InfluxDB using host %s", ch->name(), _host.c_str());
	} catch (vz::OptionNotFoundException &e) {
		print(log_error, "api InfluxDB requires parameter \"host\"!", ch->name());
		throw;
	} catch (vz::VZException &e) {
		print(log_error, "api InfluxDB requires parameter \"host\" as string!", ch->name());
		throw;
	}

try {
		_username = optlist.lookup_string(pOptions, "username");
		print(log_finest, "api InfluxDB using username %s",ch->name(), _username.c_str());
	} catch (vz::OptionNotFoundException &e) {
		//print(log_error, "api InfluxDB requires parameter \"username\"!", ch->name());
		//throw;
		print(log_debug, "api InfluxDB no username set", ch->name());
		_username = "";
	} catch (vz::VZException &e) {
		print(log_error, "api InfluxDB requires parameter \"username\" as string!", ch->name());
		throw;
	}

try {
		_password = optlist.lookup_string(pOptions, "password");
		//TODO: remove logging of password
		print(log_finest, "api InfluxDB using password %s", ch->name(), _password.c_str());
	} catch (vz::OptionNotFoundException &e) {
		//print(log_error, "api InfluxDB requires parameter \"password\"!", ch->name());
		//throw;
		print(log_debug, "api InluxDB no password set", ch->name());
		_password = "";
	} catch (vz::VZException &e) {
		print(log_error, "api InfluxDB requires parameter \"password\" as string!", ch->name());
		throw;
	}

try {
		_database = optlist.lookup_string(pOptions, "database");
		print(log_finest, "api InfluxDB using database %s", ch->name(), _database.c_str());
	} catch (vz::OptionNotFoundException &e) {
		print(log_debug, "api InfluxDB will use default database \"vzlogger\"", ch->name());
		_database = "vzlogger";
	} catch (vz::VZException &e) {
		print(log_error, "api InfluxDB requires parameter \"database\" as string!", ch->name());
		throw;
	}

try {
		_measurement_name = optlist.lookup_string(pOptions, "measurement_name");
		print(log_finest, "api InfluxDB using measurement name %s", ch->name(), _database.c_str());
	} catch (vz::OptionNotFoundException &e) {
		print(log_debug, "api InfluxDB will use default measurement name \"vzlogger\"", ch->name());
		_measurement_name = "vzlogger";
	} catch (vz::VZException &e) {
		print(log_error, "api InfluxDB requires parameter \"measurement_name\" as string!", ch->name());
		throw;
	}

try {
		_curl_timeout = optlist.lookup_int(pOptions, "timeout");
		print(log_finest, "api InfluxDB using curl timeout %i", ch->name(), _curl_timeout);
	} catch (vz::OptionNotFoundException &e) {
		_curl_timeout = 30;  //seconds
		print(log_debug, "api InfluxDB will use default timeout %i", ch->name(), _curl_timeout);
	} catch (vz::VZException &e) {
		print(log_error, "api InfluxDB requires parameter \"timeout\" as int!", ch->name());
		throw;
	}


	// build request url
	_url = _host;
	_url.append("/write");
	_url.append("?db=");
	_url.append(_database);
	print(log_debug, "api InfluxDB using url %s", ch->name(), _url.c_str());

}

vz::api::InfluxDB::~InfluxDB() // destructor
{
}

void vz::api::InfluxDB::send()
{
	CURLresponse response;
	long int http_code;
	CURLcode curl_code;
	std::string request_body;
	Buffer::Ptr buf = channel()->buffer();
	Buffer::iterator it;

	_api.curl = curlSessionProvider ? curlSessionProvider->get_easy_session(channel()->uuid()) : 0; // TODO add option to use parallel sessions. Simply add uuid() to the key.
	if (!_api.curl) {
		throw vz::VZException("CURL: cannot create handle.");
	}

	print(log_finest, "Buffer has %i items", channel()->name(), buf->size());
	buf->lock();
	for (it = buf->begin(); it != buf->end(); it++) {
		print(log_finest, "Reading buffer: timestamp %lld value %f", channel()->name(), it->time_ms(), it->value());
		request_body.append(_measurement_name);
		request_body.append(",");
		request_body.append("uuid=");
		request_body.append(channel()->uuid());
		request_body.append(" ");
		request_body.append("value=");
		request_body.append(std::to_string(it->value()));
		request_body.append(" ");
		request_body.append(std::to_string(it->time_ms()));
		request_body.append("000000"); // needed for correct InfluxDB timestamp
		request_body.append("\n");
		it->mark_delete();
	}
	buf->unlock();
	print(log_info, "request body is %s", channel()->name(), request_body.c_str());

	curl_easy_setopt(_api.curl, CURLOPT_URL, _url.c_str());
	//curl_easy_setopt(_api.curl, CURLOPT_HTTPHEADER, _api.headers);
	curl_easy_setopt(_api.curl, CURLOPT_VERBOSE, options.verbosity());
	curl_easy_setopt(_api.curl, CURLOPT_DEBUGFUNCTION, vz::api::InfluxDB::curl_custom_debug_callback);
	curl_easy_setopt(_api.curl, CURLOPT_DEBUGDATA, channel().get());

	// signal-handling in libcurl is NOT thread-safe. so force to deactivated them!
	//TODO: this causes block on error
	curl_easy_setopt(_api.curl, CURLOPT_NOSIGNAL, 1);

	// set timeout to 5 sec. required if next router has an ip-change.
	curl_easy_setopt(_api.curl, CURLOPT_TIMEOUT, _curl_timeout);

	curl_easy_setopt(_api.curl, CURLOPT_POSTFIELDS, request_body.c_str());
	//curl_easy_setopt(_api.curl, CURLOPT_WRITEFUNCTION, curl_custom_write_callback);
	curl_easy_setopt(_api.curl, CURLOPT_WRITEDATA, (void *) &response);

	curl_code = curl_easy_perform(_api.curl);
  print(log_debug, "Influxdb curl terminated", channel()->name());
	curl_easy_getinfo(_api.curl, CURLINFO_RESPONSE_CODE, &http_code);
	//print(log_debug, "InfluxDB CURL success", channel()->name());

	if (curl_code == CURLE_OK && (http_code == 200 || http_code == 204)) { // everything is ok
		print(log_debug, "InfluxDB CURL success", channel()->name());
	  buf->clean();
	}
	else {
		buf->undelete();
		print(log_error, "InfluxDB CURL error!", channel()->name());
	}

	if (curlSessionProvider)
		curlSessionProvider->return_session(channel()->uuid(), _api.curl);
}

int vz::api::InfluxDB::curl_custom_debug_callback(
	CURL *curl
	, curl_infotype type
	, char *data
	, size_t size
	, void *arg
	) {
	Channel *ch = static_cast<Channel *> (arg);
	char *end = strchr(data, '\n');

	if (data == end) return 0; // skip empty line

	switch (type) {
			case CURLINFO_TEXT:
			case CURLINFO_END:
				if (end) *end = '\0'; // terminate without \n
				print((log_level_t)(log_debug+5), "CURL: %.*s", ch->name(), (int) size, data);
				break;

			case CURLINFO_SSL_DATA_IN:
			case CURLINFO_DATA_IN:
				print((log_level_t)(log_debug+5), "CURL: Received %lu bytes", ch->name(), (unsigned long) size);
				print((log_level_t)(log_debug+5), "CURL: Received '%s' bytes", ch->name(), data);
				break;

			case CURLINFO_SSL_DATA_OUT:
			case CURLINFO_DATA_OUT:
				data[size]=0;
				print((log_level_t)(log_debug+5), "CURL: Sent %lu bytes.. ", ch->name(), (unsigned long) size);
				print((log_level_t)(log_debug+5), "CURL: Sent '%s' bytes", ch->name(), data);
				break;

			case CURLINFO_HEADER_IN:
			case CURLINFO_HEADER_OUT:
				break;
	}

	return 0;
}

void vz::api::InfluxDB::register_device()
{
}
