/*
 * Author: Matthias Behr, mbehr (a) mcbehr dot de
 * */

#ifndef __push_data_hpp_
#define __push_data_hpp_

#include <cstdint>
#include <list>
#include <pthread.h>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility> // for std::pair

// PushDataList provides a thread safe list
class PushDataList {
  public:
	typedef std::pair<int64_t, double> DataTuple;
	typedef std::queue<DataTuple> DataQueue;
	typedef std::unordered_map<std::string, DataQueue> DataMap;

	PushDataList();
	~PushDataList();
	void add(const std::string &uuid, const int64_t &time_ms, const double &value);
	DataMap *waitForData(); // blocks infinite until data is available. returned object is owned by
							// caller! must be deleted after usage!
  protected:
	DataMap *_next;
	pthread_mutex_t _map_mutex;
	pthread_cond_t _cond;
};

// var to a global/single instance. needs to be initialzed e.g. in main()
extern PushDataList *pushDataList;

class PushDataServer {
  public:
	PushDataServer(struct json_object *option);
	PushDataServer(const PushDataServer &) = delete; // no copy constructor!
	~PushDataServer();
	bool waitAndSendOnceToAll();

  protected:
	typedef struct {
		char *data;
		size_t size;
	} CURLresponse;

	std::string generateJson(PushDataList::DataMap &dataMap);
	bool send(const std::string &middleware, const std::string &datastr);
	friend class PushDataServerTest;

	static size_t curl_custom_write_callback(void *ptr, size_t size, size_t nmemb, void *data);

	typedef std::list<std::string> MiddlewareList;
	MiddlewareList _middlewareList;
	struct curl_slist *_headers;
};

void *push_data_thread(void *arg);
void end_push_data_thread(); // notifies the thread to stop, does not wait/join for the thread!

#endif
