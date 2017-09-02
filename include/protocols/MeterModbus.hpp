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
public:
	ModbusRTUConnection(const std::string &device, int baud);
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

class ModbusReadingIdentifier : public ReadingIdentifier {
	MeterModbus::slaveid_t _slaveid;
	std::string _name;
	void parse(const std::string& s);
public:
	ModbusReadingIdentifier()
	:_slaveid(0) {}
	ModbusReadingIdentifier(MeterModbus::slaveid_t slave, const std::string& name)
	: _slaveid(slave), _name(name) {}
	ModbusReadingIdentifier(const std::string& conf);
	virtual ~ModbusReadingIdentifier(){};

	virtual size_t unparse(char *buffer, size_t n);
	virtual bool operator==( ReadingIdentifier const &cmp) const;

	virtual const std::string toString();

};
#endif /* METERMODBUS_H_ */
