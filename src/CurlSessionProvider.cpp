/**
 * CurlSessionProvider - provides one curl session (easy handle) for a key
 *
 * @author Matthias Behr <mbehr@mcbehr.de>
 * @copyright Copyright (c) 2015 - 2023, The volkszaehler.org project
 * @package vzlogger
 * @license http://opensource.org/licenses/gpl-license.php GNU Public License
 */

#include "CurlSessionProvider.hpp"
#include <assert.h>
#include <time.h>
#include <unistd.h>

CurlSessionProvider::CurlSessionProvider() {
	_map_mutex = PTHREAD_MUTEX_INITIALIZER;
	curl_global_init(CURL_GLOBAL_ALL);
}

CurlSessionProvider::~CurlSessionProvider() {
	// curl_easy_cleanup for each CURL*
	unsigned inUseRetry = 5;
	do {
		timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += 1;
		pthread_mutex_timedlock(&_map_mutex,
								&ts); // try max 1s to acquire the lock. There might have been
									  // another thread pthread_cancelled while owning the lock.
		// check whether any is still inUse:
		bool inUse = false;
		for (map_it it = _easy_handle_map.begin(); it != _easy_handle_map.end(); ++it) {
			CurlUsage cu = (*it).second;
			if (cu.inUse) {
				inUse = true;
			}
		}
		if (!inUse or inUseRetry == 1) {
			inUseRetry = 0;
			for (map_it it = _easy_handle_map.begin(); it != _easy_handle_map.end(); ++it) {
				CurlUsage cu = (*it).second;
				curl_easy_cleanup(cu.eh);
			}
			curl_global_cleanup();
		}
		pthread_mutex_unlock(&_map_mutex);
		if (inUseRetry > 0) {
			sleep(1); // have to do this without holding mutex
			inUseRetry--;
		}
	} while (inUseRetry > 0);
	pthread_mutex_destroy(&_map_mutex);
}

// thread-safe functions:
CURL *CurlSessionProvider::get_easy_session(
	std::string key, int timeout) // this is intended to block if the handle for the current key is
								  // in use and single_session_per_key
{
	CURL *toRet = 0;
	// thread safe lock here to access the map:
	assert(0 == pthread_mutex_lock(&_map_mutex));
	map_it it = _easy_handle_map.find(key);
	if (it != _easy_handle_map.end()) {
		CurlUsage &cur = (*it).second;
		pthread_mutex_unlock(
			&_map_mutex); // we unlock here already but access the current element anyhow assuming
						  // an insert doesnt invalidate the reference
		int err;
		do {
			timespec abs_time;
			clock_gettime(CLOCK_REALTIME, &abs_time);
			abs_time.tv_sec += 1;
			pthread_testcancel(); // to check whether the thread shall end here!
			err = pthread_mutex_timedlock(&cur.mutex, &abs_time);
		} while ((err == EAGAIN) || (err == ETIMEDOUT));
		assert(!cur.inUse);
		cur.inUse = true;
		toRet = cur.eh;
	} else {
		// create new one:
		CurlUsage cu;
		cu.eh = curl_easy_init();
		cu.inUse = true;
		pthread_mutex_lock(&cu.mutex);
		_easy_handle_map.insert(std::make_pair(key, cu));
		toRet = cu.eh;
		pthread_mutex_unlock(&_map_mutex);
	}

	return toRet;
}

void CurlSessionProvider::return_session(
	std::string key, CURL *&eh) // return a handle. this unblocks another pending request for this
								// key if single_session_per_key
{
	// thread safe lock here:
	assert(0 == pthread_mutex_lock(&_map_mutex));
	CurlUsage &cu = _easy_handle_map[key];
	assert(eh == cu.eh);
	eh = 0;
	cu.inUse = false;
	pthread_mutex_unlock(&cu.mutex);
	pthread_mutex_unlock(&_map_mutex);
}

bool CurlSessionProvider::inUse(std::string key) {
	pthread_mutex_lock(&_map_mutex);
	cmap_it it = _easy_handle_map.find(key);
	if (it != _easy_handle_map.end()) {
		pthread_mutex_unlock(&_map_mutex);
		return (*it).second.inUse;
	}
	pthread_mutex_unlock(&_map_mutex);
	return false;
}

// global var:
CurlSessionProvider *curlSessionProvider = 0;
