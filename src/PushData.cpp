/*
 * Author: (c) Matthias Behr, mbehr (a) mcbehr dot de
 * */

#include "PushData.hpp"
#include "CurlSessionProvider.hpp"
#include "vzlogger.h"
#include <assert.h>
#include <time.h>

PushDataServer::PushDataServer(struct json_object *option) : _headers(0) {
	if (option) {
		// todo parse param option (is a json_type_array with len>0
		// expected is each array item to be an object with key "url"
		assert(json_object_get_type(option) == json_type_array);
		int len = json_object_array_length(option);
		assert(len > 0);
		for (int i = 0; i < len; ++i) {
			struct json_object *jso = json_object_array_get_idx(option, i);
			if (json_object_get_type(jso) != json_type_object)
				throw vz::VZException("config: push array element not an object");
			struct json_object *jv;
			if (!json_object_object_get_ex(jso, "url", &jv))
				throw vz::VZException("config: push url not found");
			if (json_object_get_type(jv) != json_type_string)
				throw vz::VZException("config: push url no string");
			std::string url = json_object_get_string(jv);
			_middlewareList.push_back(url);
		}

	} // else for now assume this as the unit testing case and accept it
	else
		print(log_error, "PushDataServer created with null option", "push");

	char agent[255];
	sprintf(agent, "User-Agent: %s/%s (%s)", PACKAGE, VERSION, curl_version());

	curl_global_init(CURL_GLOBAL_ALL);
	_headers = curl_slist_append(_headers, "Content-type: application/json");
	_headers = curl_slist_append(_headers, "Accept: application/json");
	_headers = curl_slist_append(_headers, agent);
}

PushDataServer::~PushDataServer() {
	if (_headers)
		curl_slist_free_all(_headers);
}

bool PushDataServer::waitAndSendOnceToAll() {
	if (!pushDataList) {
		print(log_error, "waitAndSendOnceToAll empty pushDataList!", "push");
		return false;
	}
	PushDataList::DataMap *dataMap = pushDataList->waitForData();
	if (!dataMap) {
		print(log_finest, "waitAndSendOnceToAll empty dataMap (timeout?)",
			  "push"); // this is no error as it happens each 5s on timeout
		return false;
	}

	std::string json = generateJson(*dataMap);

	// now send this data to all defined push middlewares:
	print(log_debug, "push: %s", "push", json.c_str());

	bool toRet = true;
	// iterate through list of push middlewares:
	for (auto it = _middlewareList.begin(); it != _middlewareList.end(); ++it) {
		// use CurlSessionProvider to serialize access to middleware:
		if (!send(*it, json))
			toRet = false;
	}

	delete dataMap;
	return toRet;
}

std::string PushDataServer::generateJson(PushDataList::DataMap &dataMap) {
	std::string toRet;
	struct json_object *jso = json_object_new_object();
	struct json_object *jsa = json_object_new_array(); // jsa will be the "data" object

	// now add a tuple (uuid, values) for each uuid:
	for (auto it = dataMap.begin(); it != dataMap.end(); ++it) {
		struct json_object *jsu = json_object_new_object();
		json_object_object_add(jsu, "uuid", json_object_new_string((*it).first.c_str()));

		struct json_object *jst = json_object_new_array();
		// now add the DataTuples to the jst:
		while (!(*it).second.empty()) {
			const PushDataList::DataTuple &t = (*it).second.front();
			struct json_object *jsv = json_object_new_array();
			json_object_array_add(jsv, json_object_new_int64(t.first));
			json_object_array_add(jsv, json_object_new_double(t.second));

			json_object_array_add(jst, jsv);
			(*it).second.pop();
		}

		json_object_object_add(jsu, "tuples", jst);

		json_object_array_add(jsa, jsu);
	}

	json_object_object_add(jso, "data", jsa);
	const char *json_str = json_object_to_json_string(jso);
	toRet = json_str;
	json_object_put(jso);

	return toRet;
}

bool PushDataServer::send(const std::string &middleware, const std::string &datastr) {
	bool toRet = true;
	CURL *curl = curlSessionProvider ? curlSessionProvider->get_easy_session(middleware) : 0;
	if (!curl) {
		print(log_alert, "send no curl session!", "push");
		return false;
	}

	CURLresponse response;
	response.size = 0;
	response.data = 0;
	CURLcode curl_code;
	long int http_code;

	curl_easy_setopt(curl, CURLOPT_URL, middleware.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, _headers);
	// curl_easy_setopt(curl, CURLOPT_VERBOSE, options.verbosity());
	curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, 0);
	curl_easy_setopt(curl, CURLOPT_DEBUGDATA, 0);
	// signal-handling in libcurl is NOT thread-safe. so force to deactivated them!
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

	// set timeout to 30 sec. required if e.g. next router has an ip-change.
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, datastr.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_custom_write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

	curl_code = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

	if (curlSessionProvider)
		curlSessionProvider->return_session(middleware, curl);

	// check response
	if (curl_code == CURLE_OK && http_code == 200) { // everything is ok
		print(log_debug, "CURL Request to %s succeeded with code: %i", "push", middleware.c_str(),
			  http_code);
	} else { // error
		if (curl_code != CURLE_OK) {
			print(log_alert, "CURL: %s %s", "push", middleware.c_str(),
				  curl_easy_strerror(curl_code));
			toRet = false;
		} else if (http_code != 200) {
			print(log_alert, "CURL Error from url %s: %d %s", "push", middleware.c_str(), http_code,
				  response.data);
			toRet = false;
		}
	}

	if (response.data)
		free(response.data);
	if (toRet)
		print(log_finest, "send ok to url %s", "push", middleware.c_str());
	else
		print(log_debug, "send nok to url %s", "push", middleware.c_str());

	return toRet;
}

size_t PushDataServer::curl_custom_write_callback(void *ptr, size_t size, size_t nmemb,
												  void *data) {
	size_t realsize = size * nmemb;
	CURLresponse *response = static_cast<CURLresponse *>(data);

	response->data = (char *)realloc(response->data, response->size + realsize + 1);
	if (response->data == NULL) { // out of memory!
		print(log_alert, "Cannot allocate memory data=%p response->size=%zu realsize=%zu", "push",
			  data, response->size, realsize);
		exit(EXIT_FAILURE);
	}

	memcpy(&(response->data[response->size]), ptr, realsize);
	response->size += realsize;
	response->data[response->size] = 0;

	return realsize;
}

PushDataList::PushDataList() : _next(0), _map_mutex(PTHREAD_MUTEX_INITIALIZER) {
	pthread_cond_init(&_cond, NULL);
}

PushDataList::~PushDataList() {
	pthread_mutex_lock(&_map_mutex); // todo. wrong thread might own the mutex? But might have been
									 // cancelled (thread_cancellation so will never unlock!)

	if (_next)
		delete _next;

	// we keep it locked to prevent race conds at the end
	pthread_cond_destroy(&_cond);
	pthread_mutex_destroy(&_map_mutex);
}

void PushDataList::add(const std::string &uuid, const int64_t &time_ms, const double &value) {
	assert(0 == pthread_mutex_lock(&_map_mutex));

	if (!_next)
		_next = new DataMap;

	_next->operator[](uuid).push(DataTuple(time_ms, value));

	pthread_cond_broadcast(&_cond);
	pthread_mutex_unlock(&_map_mutex);
}

PushDataList::DataMap *PushDataList::waitForData() {
	DataMap *toRet = 0;
	assert(0 == pthread_mutex_lock(&_map_mutex));
	int rc = 0;

	// try max 5s. We need to avoid deadlocking e.g. on program end/termination.
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 5;

	while ((!(_next && !_next->empty())) && rc == 0) {
		rc = pthread_cond_timedwait(&_cond, &_map_mutex, &ts);
	}

	if (rc == 0) {
		toRet = _next;
		_next = 0; // change ownership to caller. We will create new one on next add()
	}
	pthread_mutex_unlock(&_map_mutex);

	return toRet;
}

// global var:
PushDataList *pushDataList = 0;
volatile bool endThread = false;

void *push_data_thread(void *arg) {
	print(log_debug, "Start push_data_thread", "push");

	PushDataServer *pds = static_cast<PushDataServer *>(arg);

	if (pds && pushDataList) {
		while (!endThread) {
			pds->waitAndSendOnceToAll();
		}
	}

	print(log_debug, "Stopped push_data_thread", "push");
	return 0;
}

void end_push_data_thread() { endThread = true; }
