/**
 * @file MeterModbus.h
 * @brief
 *  Created on: Apr 25, 2016
 * @author Florian Achleitner <florian.achleitner@student.tugraz.at>
 */
#ifndef METERMODBUS_H_
#define METERMODBUS_H_

#include "protocols/Protocol.hpp"
#include <modbus/modbus.h>
#include <errno.h>
#include <map>

class ModbusException : public std::runtime_error {
	int _errno;
	std::string _what;
public:
	explicit ModbusException(const std::string& arg);
	virtual const char *what() const noexcept override {
		return _what.c_str();
	}

};

class ModbusConnection
{
protected:
	modbus_t *_ctx;
	bool _connected;
public:
	typedef vz::shared_ptr<ModbusConnection> Ptr;
	ModbusConnection() :
		_ctx(NULL), _connected(false) {}
	virtual ~ModbusConnection();
	modbus_t *getctx() {
		return _ctx;
	}

	virtual void open() = 0;
	virtual void connect();
	virtual void close();
	virtual void read_registers(int addr, int nb, uint16_t *dest, unsigned slave);
	virtual void debug(bool enable);
private:
	ModbusConnection(const ModbusConnection&) = delete;
	ModbusConnection& operator= (const ModbusConnection&) = delete;
};

class ModbusRTUConnection : public ModbusConnection
{
	std::string _device;
	int _baud;
public:
	ModbusRTUConnection(const std::string &device, int baud)
	: _device(device), _baud(baud) {}
	virtual void open();
};

class ModbusTCPConnection : public ModbusConnection
{
	std::string _ip;
	int _port;
public:
	ModbusTCPConnection(const std::string &ip, int port = 502)
	: _ip(ip), _port(port) {}
	virtual void open();
};

class RegisterMap
{
public:
	typedef vz::shared_ptr<RegisterMap> Ptr;
	virtual ~RegisterMap() {}
	virtual void read(std::vector<Reading>& rds, ModbusConnection::Ptr conn, unsigned id) = 0;
	static Ptr findMap(const std::string &name) {
		return maps.at(name)();
	}
private:
	template <class T> static Ptr createMap() {
		return Ptr(new T());
	}

	static std::map<std::string, Ptr (*)()> maps;
};
/**
 *
 */
class MeterModbus: public vz::protocol::Protocol
{
public:
	typedef unsigned slaveid_t;
private:
	ModbusConnection::Ptr _mbconn;
	bool _libmodbus_debug;
	typedef std::map<slaveid_t, RegisterMap::Ptr> SlaveRegMaps;
	SlaveRegMaps _devices;

	void create_rtu(const std::list<Option> &options);
	void create_tcp(const std::list<Option> &options);
public:
	MeterModbus(const std::list<Option> &options);
	virtual ~MeterModbus();
	virtual int open();
	virtual int close();
	virtual ssize_t read(std::vector<Reading> &rds, size_t n);
};


class OpRegisterMap: public RegisterMap {
public:
	virtual ~OpRegisterMap() {}
	virtual void read(std::vector<Reading>& rds, ModbusConnection::Ptr conn, unsigned id);
};

class IMEmeterRegisterMap: public RegisterMap {
public:
	virtual ~IMEmeterRegisterMap() {}
	virtual void read(std::vector<Reading>& rds, ModbusConnection::Ptr conn, unsigned id);
};

#endif /* METERMODBUS_H_ */
