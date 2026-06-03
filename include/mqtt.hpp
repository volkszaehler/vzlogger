/*
 * Author: Matthias Behr, mbehr (a) mcbehr dot de
 * (c) 2018
 * */

#ifndef __mqtt_hpp_
#define __mqtt_hpp_

#include <string>

struct mosquitto; // forward decl. to avoid pulling the header here

class MqttClient {
  public:
	MqttClient(struct json_object *option);
	MqttClient() = delete;                   // no default constr.
	MqttClient(const MqttClient &) = delete; // no copy constr.
	~MqttClient();
	bool isConfigured() const;

	// thread safe, non blocking. Used by the per-channel mqtt api (vz::api::MQTT).
	void publish(const std::string &topic, const std::string &payload);

	// global defaults from the "mqtt" config block, used by the api as fallback:
	const std::string &topicPrefix() const { return _topic; } // ends with '/'
	int qos() const { return _qos; }
	bool retain() const { return _retain; }
	bool timestamp() const { return _timestamp; }
	bool rawAndAgg() const { return _rawAndAgg; }

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
};

extern MqttClient *mqttClient;

void *mqtt_client_thread(void *arg);
void end_mqtt_client_thread(); // notifies the thread to stop. does not wait for the thread

#endif
