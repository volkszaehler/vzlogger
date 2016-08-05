/**
 * @file MeterModbus.cpp
 * @brief
 *  Created on: Apr 25, 2016
 * @author Florian Achleitner <florian.achleitner@student.tugraz.at>
 */
#include "protocols/MeterModbus.hpp"

MeterModbus::MeterModbus(const std::list<Option> &options) :
Protocol("modbus"), _libmodbus_debug(false)
{
	std::string mb_type = lookup_mandatory<const char *, OptionList::lookup_string>(options, "type", name());

	if (mb_type == "rtu")
		create_rtu(options);
	else if (mb_type == "tcp")
		create_tcp(options);
	else
		throw(std::invalid_argument("Modbus type can be rtu or tcp."));

	_libmodbus_debug = lookup_optional<bool, OptionList::lookup_bool>(options, "libmodbus_debug", false);

	std::string map = lookup_mandatory<const char *, OptionList::lookup_string>(options, "register_map", name());

	try {
	_regmap = RegisterMap::findMap(map);
	} catch (std::out_of_range &e) {
		throw(std::invalid_argument("Modbus register map invalid."));
	}
}

void MeterModbus::create_rtu(const std::list<Option> &options) {
	std::string rtu_device;
	int rtu_baudrate;
	int rtu_slave;

	rtu_device = lookup_mandatory<const char *, OptionList::lookup_string>(options, "device", name());
	rtu_baudrate = lookup_optional<int, OptionList::lookup_int>(options, "baudrate", 9600);
	rtu_slave = lookup_optional<int, OptionList::lookup_int>(options, "slave", 1);

	_mbconn.reset(new ModbusRTUConnection(rtu_device, rtu_baudrate, rtu_slave));
}

void MeterModbus::create_tcp(const std::list<Option> &options) {
	std::string tcp_host;
	int tcp_port;
	tcp_host = lookup_mandatory<const char *, OptionList::lookup_string>(options, "host", name());
	tcp_port = lookup_optional<int, OptionList::lookup_int>(options, "port", 502);

	_mbconn.reset(new ModbusTCPConnection(tcp_host, tcp_port));
}

MeterModbus::~MeterModbus()
{
	// TODO Auto-generated destructor stub
}

int MeterModbus::open() {
	_mbconn->connect();
	if (_libmodbus_debug)
		modbus_set_debug(_mbconn->getctx(), TRUE);
	return SUCCESS;
}

int MeterModbus::close() {
	_mbconn.reset();
	return SUCCESS;
}

ssize_t MeterModbus::read(std::vector<Reading> &rds, size_t n) {
	try {
		rds = _regmap->read(_mbconn);
		return rds.size();
	} catch (ModbusException &e) {
		print(log_error, "Modbus read error: %s", name().c_str(), e.what());
		return 0;
	}

}

void ModbusConnection::connect() {
	int ret = modbus_connect(ctx);
	if (ret < 0) {
		throw ModbusException("connecting modbus");
	}

}
ModbusConnection::~ModbusConnection() {
	if (ctx) {
		modbus_close(ctx);
		modbus_free(ctx);
		ctx = NULL;
	}
}

ModbusRTUConnection::ModbusRTUConnection(const std::string &device, int baud, int slave) {
	ctx = modbus_new_rtu(device.c_str(), baud, 'N', 8, 1);
	if (ctx == NULL)
		throw ModbusException("creating new rtu");
	modbus_set_slave(ctx, slave);
}

ModbusTCPConnection::ModbusTCPConnection(const std::string &ip, int port) {
	ctx = modbus_new_tcp(ip.c_str(), port);
	if (ctx == NULL)
		throw ModbusException("creating new tcp");
}


std::map<std::string, RegisterMap::Ptr (*)()> RegisterMap::maps = {
		{ "ohmpilot", createMap<OpRegisterMap> },
		{ "imemeter", createMap<IMEmeterRegisterMap> }
};

std::vector<Reading> OpRegisterMap::read(ModbusConnection::Ptr conn) {
	std::vector<Reading> rds;
	uint16_t regs[10];
	int ret;
	ret = modbus_read_registers(conn->getctx(), 40799, 10, regs);
	if (ret < 0)
		throw(ModbusException("OP read failed."));

	rds.push_back(Reading(regs[9] / 10.0, new StringIdentifier("T")));

	rds.push_back(Reading(regs[2], new StringIdentifier("P")));

	rds.push_back(Reading(regs[8], new StringIdentifier("E")));

	return rds;
}

std::vector<Reading> IMEmeterRegisterMap::read(ModbusConnection::Ptr conn) {
	std::vector<Reading> rds;
	int ret;
	const int reg_offset = 4096;
	const int reg_len = 59;
	uint16_t regs[reg_len];
	int value;

	ret = -3;
	ret = modbus_read_registers(conn->getctx(), reg_offset, reg_len, regs);
	if (ret < 0)
		throw(ModbusException("IME meter read failed."));

	value = MODBUS_GET_INT32_FROM_INT16(regs, 4116 - reg_offset);
	rds.push_back(Reading(value * 0.01, new StringIdentifier("Current Power")));

	value = MODBUS_GET_INT32_FROM_INT16(regs, 4128 - reg_offset);
	rds.push_back(Reading(value, new StringIdentifier("TotalExpWh")));

	return rds;
}

