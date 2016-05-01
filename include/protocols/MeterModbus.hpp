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
	modbus_t *ctx;
public:
	typedef vz::shared_ptr<ModbusConnection> Ptr;
	ModbusConnection(const char *device, int baud, int slave);
	virtual ~ModbusConnection();
	modbus_t *getctx() {
		return ctx;
	}

};

class ModbusRTUConnection : public ModbusConnection
{

};

class ModbusTCPConnection : public ModbusConnection
{

};

/**
 *
 */
class MeterModbus: public vz::protocol::Protocol
{
	ModbusConnection::Ptr _mbconn;
	const char *_device;
	int _baudrate;
	bool _libmodbus_debug;
	int _slave;

public:
	MeterModbus(const std::list<Option> &options);
	virtual ~MeterModbus();
	virtual int open();
	virtual int close();
	virtual ssize_t read(std::vector<Reading> &rds, size_t n);
};

#endif /* METERMODBUS_H_ */
