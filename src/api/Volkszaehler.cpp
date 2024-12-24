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

#ifndef VZ_PICO
# include <curl/curl.h>
#else // VZ_PICO
# include <lwip/init.h>
#endif // VZ_PICO

#include <json-c/json.h>
#include <math.h>
#include <stddef.h>
#include <sys/time.h>
#include <unistd.h>

#include "Config_Options.hpp"
#ifdef VZ_PICO
# include <pico/stdlib.h>
#else // VZ_PICO
# include "CurlSessionProvider.hpp"
#endif // VZ_PICO
#include <VZException.hpp>
#include <api/Volkszaehler.hpp>

extern Config_Options options;

const int MAX_CHUNK_SIZE = 64;

vz::api::Volkszaehler::Volkszaehler(Channel::Ptr ch, std::list<Option> pOptions)
	: ApiIF(ch), _last_timestamp(0), _lastReadingSent(0)
#ifdef VZ_PICO
        ,_api(NULL)
#endif // VZ_PICO
{
	OptionList optlist;
	char agent[255];

        print(log_debug, "Creating Volkszaehler API instance", ch->name());

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
	_url = _middleware;
	_url.append("/data/");
	_url.append(channel()->uuid());
	_url.append(".json");

        print(log_debug, "Volkszaehler API: %s (timeout %d)", ch->name(), _url.c_str(), _curlTimeout);

#ifdef VZ_PICO
        // Something like https://vz-server:8000/middleware.php"
        char prot[10], h[128], url[512], baseUrl[256];
        uint p;
        if(sscanf(_middleware.c_str(), "%[^:]://%[^:]:%d/%s", prot, h, &p, url) == 4)
        {
          sprintf(baseUrl, "%s://%s:%d", prot, h, p);
        }
        else if(sscanf(_middleware.c_str(), "%[^:]://%[^:]/%s", prot, h, url) == 3)
        {
          sprintf(baseUrl, "%s://%s:%d", prot, h, (strcmp(prot, "https") == 0 ? 443: 80));
        }
        else
        {
          throw vz::VZException("VZ-API: Cannot parse URL %s.", _middleware.c_str());
        }

        _api = vz::api::LwipIF::getInstance(baseUrl, _curlTimeout);

	sprintf(agent, "User-Agent: %s/%s (LwIP %s)", PACKAGE, VERSION, LWIP_VERSION_STRING); // build user agent
	_api->addHeader("Content-type: application/json");
	_api->addHeader("Accept: application/json");
	_api->addHeader("Connection: keep-alive");
	_api->addHeader(agent);

        print(log_debug, "Volkszaehler API connecting %s ...", ch->name(), baseUrl);
        _api->connect();

        print(log_debug, "Volkszaehler API connection initiated.", ch->name());

#else // VZ_PICO
	sprintf(agent, "User-Agent: %s/%s (%s)", PACKAGE, VERSION, curl_version()); // build user agent
	_api.headers = NULL;
	_api.headers = curl_slist_append(_api.headers, "Content-type: application/json");
	_api.headers = curl_slist_append(_api.headers, "Accept: application/json");
	_api.headers = curl_slist_append(_api.headers, agent);
#endif // VZ_PICO

  response.size = 0;
  response.data = NULL;
}

vz::api::Volkszaehler::~Volkszaehler() {
	if (_lastReadingSent)
		delete _lastReadingSent;

#ifdef VZ_PICO
  delete _api;
#endif // VZ_PICO

  free(response.data);
}

void vz::api::Volkszaehler::send()
{
	long int http_code = 0;

        print(log_debug, "Volkszaehler API sending data ...", channel()->name());

#ifndef VZ_PICO
	CURLcode curl_code;
#endif // VZ_PICO

        errMsg = "OK";
        uint errOK = 0;
        uint errCode = 0;

#ifdef VZ_PICO
    // Throws exception
    uint state = _api->getState();
    if((state == VZ_SRV_CONNECTING) && ((time(NULL) - _api->getConnectInit()) > (_curlTimeout * 2)))
    {
      print(log_debug, "Volkszaehler API connecting timed out (%d).", channel()->name(), (_curlTimeout * 2));
      state = VZ_SRV_INIT;
    }
    
    if(state == VZ_SRV_INIT)
    {
      // May happen, if the server closed the connection. Reconnect ...
      print(log_debug, "Volkszaehler API reconnecting ...", channel()->name());
      _api->reconnect();

      // That will happen asynchronously, so cannot send anything right now
      return;
    }

    if(state != VZ_SRV_READY)
    {
      print(log_debug, "NOT sending request - API in state %s", channel()->name(), _api->stateStr());
      return;
    }
#endif // VZ_PICO

    if(channel()->buffer()->size() == 0)
    {
      print(log_debug, "No data to send.", channel()->name());
      return;
    }

    api_json_tuples(channel()->buffer());
    const char * json_str = outputData.c_str();
	if (json_str == NULL || strcmp(json_str, "null") == 0) {
		print(log_debug, "JSON request body is null. Nothing to send now.", channel()->name());
		return;
	}

#ifdef VZ_PICO
    // If we are here, the API is ready and there is something to send - do it

    print(log_info, "POSTing %d tuples ...", channel()->name(), _values.size());
    state =_api->postRequest(json_str, _url.c_str());
    print(log_debug, "POST request in state %d", channel()->name(), state);
    if(state == VZ_SRV_RETRY)
    {
      print(log_info, "POSTing to be retried: %d", channel()->name(), state);
      _api->setState(VZ_SRV_READY);
      return;
    }

    while(_api->getState() == VZ_SRV_SENDING)
    {
      print(log_debug, "Waiting for response ...", channel()->name());
      sleep_ms(1000);
    }

    // May be READY after receiving data || INIT after the server closed the connection
    // Anything else means, we tried to send something but no idea whether it arrived or not ...
    state = _api->getState();
    if(state != VZ_SRV_READY && state != VZ_SRV_INIT)
    {
      print(log_warning, "Sending/response timed out - API in state %s", channel()->name(), _api->stateStr());
      // Cause a reconnect at next cycle:
      _api->setState(VZ_SRV_INIT);

      // TODO Do we need to drop the data to avoid duplicates?
      print(log_debug, "Dropping %d values", channel()->name(), _values.size());
      _values.clear();

      return;
    }

    const char * resp = _api->getResponse();
    if(resp == NULL || strlen(resp) == 0)
    {
      print(log_error, "NULL response", channel()->name());
      return;
    }

    // Scan and remove headers
    uint num;
    char buf[128], buf2[128];
    while(strlen(resp) > 0)
    {
      if(resp[0] == '\r' && resp[1] == '\n')
      {
        print(log_debug, "HTTP empty line", channel()->name());
      }
      else if(sscanf(resp, "HTTP/%*d.%*d %d %[^\n]\n", &http_code, buf) == 2)
      {
        print(log_debug, "HTTP result: %d %s", channel()->name(), http_code, buf);
      }
      else if((sscanf(resp, "%[^:]: %[^\n]\n", buf, buf2) == 2) && (buf[0] != '{')) // curly brace confuses vi }
      {
        print(log_debug, "HTTP header: %s %s", channel()->name(), buf, buf2);
      }
      else
      {
        print(log_debug, "HTTP response payload: %s", channel()->name(), resp);
        // ORIG response.data = strdup(resp);
        if(response.size <= strlen(resp))
        {
          response.size = (strlen(resp) + 1);
          response.data = (char *) realloc(response.data, response.size);
        }
        strcpy(response.data, resp);
        break;
      }
  
      resp = strchr(resp, '\n') + 1;
    }

#else // VZ_PICO
	_api.curl = curlSessionProvider ? curlSessionProvider->get_easy_session(_middleware)
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

        errOK   = CURLE_OK;
        errCode = curl_code;
        errMsg  = curl_easy_strerror(curl_code);

#endif // VZ_PICO

	// check response
	if (errCode == errOK && http_code == 200) { // everything is ok
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
		if (errCode != errOK) {
			print(log_alert, "CURL: %s", channel()->name(), errMsg.c_str());
		} else if (http_code != 200) {
			char err[255];
			api_parse_exception(response, err, 255);
			print(log_alert, "CURL Error from middleware: %s", channel()->name(), err);
		}
	}

	// householding
#ifndef VZ_PICO
	free(response.data);
#endif // not VZ_PICO

	if ((errCode != errOK || http_code != 200)) {
		print(log_info, "Waiting %i secs for next request due to previous failure",
			  channel()->name(), options.retry_pause());
#ifdef VZ_PICO
  sleep_ms(options.retry_pause() * 1000);
#else // VZ_PICO
		sleep(options.retry_pause());
#endif // VZ_PICO
	}

  print(log_finest, "Volkszaehler API sending data complete.", channel()->name());
}

#ifdef VZ_PICO // Otherwise use base-class method
bool vz::api::Volkszaehler::isBusy() const
{
  uint state = _api->getState();
  return (state != VZ_SRV_READY && state != VZ_SRV_INIT);
}
#endif // VZ_PICO

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

  print(log_debug, "OutputData capacity: %d", channel()->name(), outputData.capacity());

  outputData = "[";
  int nrTuples = 0;
  for (it = _values.begin(); it != _values.end(); it++)
  {
    outputData += "[";
    outputData += std::to_string(it->time_ms());
    outputData += ",";
    outputData += std::to_string(it->value());
    outputData += "]";

    if(++nrTuples < _values.size())
    {
      outputData += ",";
    }

    if (nrTuples >= MAX_CHUNK_SIZE)
    {
      break;
    }
  }
  outputData += "]";
  print(log_debug, "copied %d/%d values for middleware transmission: %s (%d)", channel()->name(),
        nrTuples, _values.size(), outputData.c_str(), outputData.capacity());

  return NULL;
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

#ifndef VZ_PICO
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
#endif // VZ_PICO
