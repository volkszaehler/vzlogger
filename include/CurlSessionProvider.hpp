#ifndef __CURL_SESSION_PROVIDER_
#define __CURL_SESSION_PROVIDER_

#include <curl/curl.h>
#include <map>
#include <pthread.h>
#include <string>

class CurlSessionProvider {
  public:
	// non thread safe:
	CurlSessionProvider();
	~CurlSessionProvider();

	// thread-safe functions:
	CURL *get_easy_session(std::string key,
						   int timeout = 0); // this is intended to block if the handle for the
											 // current key is in use and single_session_per_key
	void return_session(std::string key, CURL *&); // return a handle. this unblocks another pending
												   // request for this key if single_session_per_key
	bool inUse(std::string key); // check whether a key is in use (does not guarantee that get...
								 // will not block)

  protected:
	class CurlUsage {
	  public:
		CurlUsage() : eh(0), inUse(false) { mutex = PTHREAD_MUTEX_INITIALIZER; };
		CURL *eh;
		bool inUse;
		pthread_mutex_t mutex;
	};

	typedef std::map<std::string, CurlUsage>::iterator map_it;
	typedef std::map<std::string, CurlUsage>::const_iterator cmap_it;

	std::map<std::string, CurlUsage> _easy_handle_map;

  private:
	pthread_mutex_t _map_mutex;
};

// var to a global/single instance. needs to be initialzed e.g. in main()
extern CurlSessionProvider *curlSessionProvider;

#endif
