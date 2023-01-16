/*
 * Author: Matthias Behr, mbehr (a) mcbehr dot de
 * (c) 2018
 * */

#ifndef __mqtt_hpp_
#define __mqtt_hpp_

#include "Channel.hpp"
#include "Reading.hpp"
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

struct mosquitto; // forward decl. to avoid pulling the header here

class MqttClient {
  public:
	MqttClient(struct json_object *option);
	MqttClient() = delete;                   // no default constr.
	MqttClient(const MqttClient &) = delete; // no copy constr.
	~MqttClient();
	bool isConfigured() const;

	void publish(Channel::Ptr ch, Reading &rds,
				 bool aggregate = false); // thread safe, non blocking
  protected:
	friend void *mqtt_client_thread(void *);
	void connect_callback(struct mosquitto *mosq, int result);
	void disconnect_callback(struct mosquitto *mosq, int result);
	void message_callback(struct mosquitto *mosq, const struct mosquitto_message *msg);

	bool _enabled;
	std::string _host;
	int _port = 0;
	int _keepalive = 10;
	std::string _user;
	std::string _pwd;
	std::string _cafile;
	std::string _capath;
	std::string _certfile;
	std::string _keyfile;
	std::string _keypass;
	bool _retain = false;
	bool _rawAndAgg = false;
	std::string _topic;
	std::string _id;
	int _qos = 0;
	bool _timestamp = false;

	bool _isConnected = false;

	struct mosquitto *_mcs = nullptr; // mosquitto client session data

	struct ChannelEntry {
		bool _announced = false;
		bool _sendRaw = true;
		bool _sendAgg = true;
		std::string _fullTopicRaw;
		std::string _fullTopicAgg;
		std::string _announceName;
		std::vector<std::pair<std::string, std::string>> _announceValues;
		void generateNames(const std::string &prefix, Channel &ch);
	};
	std::mutex _chMapMutex;
	std::unordered_map<std::string, ChannelEntry> _chMap;
};

extern MqttClient *mqttClient;

void *mqtt_client_thread(void *arg);
void end_mqtt_client_thread(); // notifies the thread to stop. does not wait for the thread

#endif
