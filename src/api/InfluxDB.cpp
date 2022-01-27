/***********************************************************************/
/** @file InfluxDB.cpp
 * This is cobbled together from the Volkszaehler and MySmartGrid API...
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

#include "Config_Options.hpp"
#include "CurlSessionProvider.hpp"
#include <VZException.hpp>
#include <api/CurlCallback.hpp>
#include <api/CurlResponse.hpp>
#include <api/InfluxDB.hpp>
#include <curl/curl.h>
#include <iomanip>
#include <sstream>
#include <stdio.h>

extern Config_Options options;

vz::api::InfluxDB::InfluxDB(const Channel::Ptr &ch, const std::list<Option> &pOptions)
	: ApiIF(ch), _response(new vz::api::CurlResponse()) {
	OptionList optlist;
	print(log_debug, "InfluxDB API initialize", ch->name());

	// parse config file options
	try {
		_host = optlist.lookup_string(pOptions, "host");
		print(log_finest, "api InfluxDB using host %s", ch->name(), _host.c_str());
	} catch (vz::OptionNotFoundException &e) {
		print(log_alert, "api InfluxDB requires parameter \"host\"!", ch->name());
		throw;
	} catch (vz::VZException &e) {
		print(log_alert, "api InfluxDB requires parameter \"host\" as string!", ch->name());
		throw;
	}

	_token_header = NULL;
	try {
		_token = optlist.lookup_string(pOptions, "token");
		print(log_finest, "api InfluxDB using login Token: %s", ch->name(), _token.c_str());
		_token = "Authorization: Token " + _token;
		_token_header = curl_slist_append(_token_header, _token.c_str());
	} catch (vz::OptionNotFoundException &e) {
		print(log_finest, "api InfluxDB no Token set", ch->name());
		_token = "";
	} catch (vz::VZException &e) {
		print(log_alert, "api InfluxDB requires parameter \"Token\" as string!", ch->name());
		throw;
	}

	try {
		_organization = optlist.lookup_string(pOptions, "organization");
		print(log_finest, "api InfluxDB using organization %s", ch->name(), _organization.c_str());
	} catch (vz::OptionNotFoundException &e) {
		print(log_finest, "api InfluxDB no organization set", ch->name());
		_organization = "";
	} catch (vz::VZException &e) {
		print(log_alert, "api InfluxDB requires parameter \"organization\" as string!", ch->name());
		throw;
	}

	try {
		_username = optlist.lookup_string(pOptions, "username");
		print(log_finest, "api InfluxDB using username %s", ch->name(), _username.c_str());
	} catch (vz::OptionNotFoundException &e) {
		print(log_finest, "api InfluxDB no username set", ch->name());
		_username = "";
	} catch (vz::VZException &e) {
		print(log_alert, "api InfluxDB requires parameter \"username\" as string!", ch->name());
		throw;
	}

	try {
		_password = optlist.lookup_string(pOptions, "password");
		// dont log passwords by default
		// print(log_finest, "api InfluxDB using password %s", ch->name(), _password.c_str());
	} catch (vz::OptionNotFoundException &e) {
		// print(log_alert, "api InfluxDB requires parameter \"password\"!", ch->name());
		// throw;
		print(log_finest, "api InfluxDB no password set", ch->name());
		_password = "";
	} catch (vz::VZException &e) {
		print(log_alert, "api InfluxDB requires parameter \"password\" as string!", ch->name());
		throw;
	}

	try {
		_database = optlist.lookup_string(pOptions, "database");
		print(log_finest, "api InfluxDB using database %s", ch->name(), _database.c_str());
	} catch (vz::OptionNotFoundException &e) {
		print(log_finest, "api InfluxDB will use default database \"vzlogger\"", ch->name());
		_database = "vzlogger";
	} catch (vz::VZException &e) {
		print(log_alert, "api InfluxDB requires parameter \"database\" as string!", ch->name());
		throw;
	}

	try {
		_measurement_name = optlist.lookup_string(pOptions, "measurement_name");
		print(log_finest, "api InfluxDB using measurement name %s", ch->name(),
			  _measurement_name.c_str());
	} catch (vz::OptionNotFoundException &e) {
		print(log_finest, "api InfluxDB will use default measurement name \"vzlogger\"",
			  ch->name());
		_measurement_name = "vzlogger";
	} catch (vz::VZException &e) {
		print(log_alert, "api InfluxDB requires parameter \"measurement_name\" as string!",
			  ch->name());
		throw;
	}

	try {
		_tags = optlist.lookup_string(pOptions, "tags");
		print(log_finest, "api InfluxDB using tags %s", ch->name(), _tags.c_str());
	} catch (vz::OptionNotFoundException &e) {
		print(log_finest, "api InfluxDB will not use any tags", ch->name());
		_tags = "";
	} catch (vz::VZException &e) {
		print(log_alert, "api InfluxDB requires parameter \"tags\" as string!", ch->name());
		throw;
	}

	try {
		_curl_timeout = optlist.lookup_int(pOptions, "timeout");
		print(log_finest, "api InfluxDB using curl timeout %i", ch->name(), _curl_timeout);
	} catch (vz::OptionNotFoundException &e) {
		_curl_timeout = 30; // seconds
		print(log_finest, "api InfluxDB will use default timeout %i", ch->name(), _curl_timeout);
	} catch (vz::VZException &e) {
		print(log_alert, "api InfluxDB requires parameter \"timeout\" as int!", ch->name());
		throw;
	}

	try {
		_max_batch_inserts = optlist.lookup_int(pOptions, "max_batch_inserts");
		print(log_finest, "api InfluxDB using max batch inserts: %i", ch->name(),
			  _max_batch_inserts);
	} catch (vz::OptionNotFoundException &e) {
		_max_batch_inserts = 4500; // max lines per request
		print(log_finest, "api InfluxDB will use default max_batch_inserts %i", ch->name(),
			  _max_batch_inserts);
	} catch (vz::VZException &e) {
		print(log_alert, "api InfluxDB requires parameter \"max_batch_inserts\" as int!",
			  ch->name());
		throw;
	}

	try {
		_max_buffer_size = optlist.lookup_int(pOptions, "max_buffer_size");
		print(log_finest, "api InfluxDB using max_buffer_size: %i", ch->name(), _max_buffer_size);
	} catch (vz::OptionNotFoundException &e) {
		_max_buffer_size = _max_batch_inserts * 100; // max items in buffer
		print(log_finest, "api InfluxDB will use default max_buffer_size %i", ch->name(),
			  _max_buffer_size);
	} catch (vz::VZException &e) {
		print(log_alert, "api InfluxDB requires parameter \"max_buffer_size\" as int!", ch->name());
		throw;
	}

	try {
		_send_uuid = optlist.lookup_bool(pOptions, "send_uuid");
		print(log_finest, "api InfluxDB using send_uuid: %s", ch->name(),
			  _send_uuid ? "true" : "false");
	} catch (vz::OptionNotFoundException &e) {
		_send_uuid = true;
		print(log_finest, "api InfluxDB will use default send_uuid %s", ch->name(),
			  _send_uuid ? "true" : "false");
	} catch (vz::VZException &e) {
		print(log_alert, "api InfluxDB requires parameter \"send_uuid\" as bool!", ch->name());
		throw;
	}

	try {
		_ssl_verifypeer = optlist.lookup_bool(pOptions, "ssl_verifypeer");
		print(log_finest, "api InfluxDB using ssl_verifypeer: %s", ch->name(),
			  _ssl_verifypeer ? "true" : "false");
	} catch (vz::OptionNotFoundException &e) {
		_ssl_verifypeer = true;
		print(log_finest, "api InfluxDB will use default ssl_verifypeer %s", ch->name(),
			  _ssl_verifypeer ? "true" : "false");
	} catch (vz::VZException &e) {
		print(log_alert, "api InfluxDB requires parameter \"_ssl_verifypeer\" as bool!",
			  ch->name());
		throw;
	}

	CURL *curlhelper = curl_easy_init();
	if (!curlhelper) {
		throw vz::VZException("CURL: cannot create handle for urlencode.");
	}
	char *database_urlencoded = curl_easy_escape(curlhelper, _database.c_str(), 0);
	if (!database_urlencoded) {
		throw vz::VZException("Cannot url-encode database name.");
	}

	// build request url
	_url = _host;
	if (!_organization.empty()) {
		char *organization_urlencoded = curl_easy_escape(curlhelper, _organization.c_str(), 0);
		if (!organization_urlencoded) {
			throw vz::VZException("Cannot url-encode organization name.");
		}
		_url.append("/api/v2/write?org=");
		_url.append(organization_urlencoded);
		_url.append("&bucket=");
	} else {
		_url.append("/write");
		_url.append("?db=");
	}
	_url.append(database_urlencoded);
	_url.append("&precision=ms");
	print(log_debug, "api InfluxDB using url %s", ch->name(), _url.c_str());
	curl_free(database_urlencoded);
}

// destructor
vz::api::InfluxDB::~InfluxDB() { curl_slist_free_all(_token_header); }

void vz::api::InfluxDB::send() {
	long int http_code;
	CURLcode curl_code;
	int request_body_lines = 0;
	std::string request_body;
	Buffer::Ptr buf = channel()->buffer();
	Buffer::iterator it;

	_api.curl =
		curlSessionProvider ? curlSessionProvider->get_easy_session(_host + channel()->uuid()) : 0;

	if (!_api.curl) {
		throw vz::VZException("CURL: cannot create handle.");
	}

	print(log_debug, "Buffer has %i items", channel()->name(), buf->size());

	// delete items if the buffer grows too large
	if (buf->size() > (unsigned)_max_buffer_size) {
		print(log_warning,
			  "Buffer too big (%i items). Deleting items. (This indicates a connection problem)",
			  channel()->name(), buf->size());
		unsigned int delta_delete =
			buf->size() - (unsigned)_max_buffer_size; // number of items to delete from buffer
		buf->lock();
		it = buf->begin();
		while (!(
			delta_delete == 0 ||
			(it ==
			 buf->end()))) { // buf->end() check shouldnt be necessary. But better safe than sorry.
			it->mark_delete();
			it++;
			delta_delete--;
		}
		buf->unlock();
		buf->clean();
		print(log_debug, "cleaned buffer, now %i items", channel()->name(), buf->size());
	}

	// build request body from buffer contents
	buf->lock();
	for (it = buf->begin(); it != buf->end(); it++) {
		if (request_body_lines >= _max_batch_inserts) {
			print(log_debug, "reached maximum lines for InfluxDB insertion request.",
				  channel()->name());
			break;
		}
		print(log_finest, "Reading buffer: timestamp %lld value %f", channel()->name(),
			  it->time_ms(), it->value());
		request_body.append(_measurement_name);
		if (_send_uuid) {
			request_body.append(",uuid=");
			request_body.append(channel()->uuid());
		}
		if (!_tags.empty()) {
			request_body.append(",");
			request_body.append(_tags);
		}
		std::stringstream value_str;
		value_str << " value=" << std::fixed << std::setprecision(6) << it->value();
		request_body.append(value_str.str());
		request_body.append(" ");
		request_body.append(std::to_string(it->time_ms()));
		request_body.append("\n"); // each measurement on new line
		it->mark_delete();
		request_body_lines++;
	}

	buf->unlock();

	if (request_body_lines > 0) { // there is something to send
		print(log_finest, "request body is %s", channel()->name(), request_body.c_str());

		_response->clear_response(); // initialize with empty response

		// if the username option is set, use curl with HTTP basic auth
		if (!_username.empty()) {
			curl_easy_setopt(_api.curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
			curl_easy_setopt(_api.curl, CURLOPT_USERNAME, _username.c_str());
			curl_easy_setopt(_api.curl, CURLOPT_PASSWORD, _password.c_str());
		} else if (_token_header) {
			curl_easy_setopt(_api.curl, CURLOPT_HTTPHEADER, _token_header);
		}
		curl_easy_setopt(_api.curl, CURLOPT_URL, _url.c_str());
		curl_easy_setopt(_api.curl, CURLOPT_VERBOSE, options.verbosity() > 0);
		curl_easy_setopt(_api.curl, CURLOPT_SSL_VERIFYPEER, _ssl_verifypeer);

		curl_easy_setopt(_api.curl, CURLOPT_DEBUGFUNCTION,
						 &(vz::api::CurlCallback::debug_callback));
		curl_easy_setopt(_api.curl, CURLOPT_DEBUGDATA, response());
		// signal-handling in libcurl is NOT thread-safe. so force to deactivated them!
		curl_easy_setopt(_api.curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(_api.curl, CURLOPT_TIMEOUT, _curl_timeout);

		curl_easy_setopt(_api.curl, CURLOPT_POSTFIELDS, request_body.c_str());
		curl_easy_setopt(_api.curl, CURLOPT_WRITEFUNCTION,
						 &(vz::api::CurlCallback::write_callback));
		curl_easy_setopt(_api.curl, CURLOPT_WRITEDATA, response());

		// actually send the request to InfluxDB
		curl_code = curl_easy_perform(_api.curl);
		print(log_finest, "Influxdb curl terminated", channel()->name());
		curl_easy_getinfo(_api.curl, CURLINFO_RESPONSE_CODE, &http_code);

		if (curl_code == CURLE_OK && http_code >= 200 && http_code < 300) { // everything is ok
			print(log_debug, "InfluxDB CURL success", channel()->name());
			buf->clean(); // delete the stuff we just sent to InfluxDB from the buffer
		} else {
			buf->undelete(); // failure to insert, so dont delete the buffer
			if (curl_code != CURLE_OK) {
				print(log_error, "CURL Error: %s", channel()->name(),
					  curl_easy_strerror(curl_code));
			}
			print(log_error, "InfluxDB error! - HTTP Status %i", channel()->name(), http_code);
			if (!_response->get_response().empty()) {
				print(log_error, "InfluxDB response was %s", channel()->name(),
					  _response->get_response().c_str());
			}
		}
	} else { // there is nothing to send
		print(log_info, "Nothing to send to InfluxDB api", channel()->name());
	}

	if (curlSessionProvider) {
		// release our curl session
		curlSessionProvider->return_session(_host + channel()->uuid(), _api.curl);
	}
}

void vz::api::InfluxDB::register_device() {
	// TODO: is this needed?
}
