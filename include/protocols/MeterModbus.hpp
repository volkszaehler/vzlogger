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

class ModbusException : public std::runtime_error {
	int _errno;
public:
	explicit ModbusException(const std::string& arg)
	: std::runtime_error(arg), _errno(errno) {}
};
class ModbusConnection
{
protected:
	modbus_t *ctx;
public:
	typedef vz::shared_ptr<ModbusConnection> Ptr;
	ModbusConnection() :
		ctx(NULL) {}
	virtual ~ModbusConnection();
	modbus_t *getctx() {
		return ctx;
	}

	virtual void connect();

};

class ModbusRTUConnection : public ModbusConnection
{
public:
	ModbusRTUConnection(const std::string &device, int baud, int slave);
};

class ModbusTCPConnection : public ModbusConnection
{
public:
	ModbusTCPConnection(const std::string &ip, int port = 502);
};

class RegisterMap
{
public:
	typedef vz::shared_ptr<RegisterMap> Ptr;
	virtual ~RegisterMap() {}
	virtual std::vector<Reading> read(ModbusConnection::Ptr conn) = 0;
};
/**
 *
 */
class MeterModbus: public vz::protocol::Protocol
{
	ModbusConnection::Ptr _mbconn;
	bool _libmodbus_debug;
	RegisterMap::Ptr _regmap;

	template <typename T, T (*L)(const std::list<Option> &, const char *)>
	T lookup_mandatory(const std::list<Option> &olist, const char *o, const char *errmsg);

	template <typename T, T (*L)(const std::list<Option> &, const char *)>
	T lookup_optional(const std::list<Option> &olist, const char *o, const T &def);

	void create_rtu(const std::list<Option> &options);
	void create_tcp(const std::list<Option> &options);
public:
	MeterModbus(const std::list<Option> &options);
	virtual ~MeterModbus();
	virtual int open();
	virtual int close();
	virtual ssize_t read(std::vector<Reading> &rds, size_t n);
};


class OhmpilotRegisterMap: public RegisterMap {
public:
	virtual ~OhmpilotRegisterMap() {}
	virtual std::vector<Reading> read(ModbusConnection::Ptr conn);
};

class IMEmeterRegisterMap: public RegisterMap {
public:
	virtual ~IMEmeterRegisterMap() {}
	virtual std::vector<Reading> read(ModbusConnection::Ptr conn);
};

#endif /* METERMODBUS_H_ */
