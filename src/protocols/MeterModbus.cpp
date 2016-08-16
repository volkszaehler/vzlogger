/**
 * @file MeterModbus.cpp
 * @brief
 *  Created on: Apr 25, 2016
 * @author Florian Achleitner <florian.achleitner@student.tugraz.at>
 */
#include "protocols/MeterModbus.hpp"

const char *ModbusException::what() const noexcept {
	std::ostringstream oss;
	oss << std::runtime_error::what() << " errno " << _errno << ": " <<  modbus_strerror(_errno) << std::endl;
	return oss.str().c_str();
}

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

	slaveid_t slaveid = 1;

	try {
		_devices[slaveid] = RegisterMap::findMap(map);
		print(log_info, "Creating MeterModbus with id %d, RegisterMap %s.", name().c_str(), slaveid, map.c_str());
		if (map == "imemeter") {
			slaveid = 2;
			_devices[slaveid] = RegisterMap::findMap(map);
			print(log_info, "Creating MeterModbus with id %d, RegisterMap %s.", name().c_str(), slaveid, map.c_str());
		}
	} catch (std::out_of_range &e) {
		throw(std::invalid_argument("Modbus register map invalid."));
	}
}

void MeterModbus::create_rtu(const std::list<Option> &options) {
	std::string  rtu_device = lookup_mandatory<const char *, OptionList::lookup_string>(options, "device", name());
	int rtu_baudrate = lookup_optional<int, OptionList::lookup_int>(options, "baudrate", 9600);
//	int rtu_slave = lookup_optional<int, OptionList::lookup_int>(options, "slave", 1);

	_mbconn.reset(new ModbusRTUConnection(rtu_device, rtu_baudrate));
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
		_mbconn->debug(true);
	return SUCCESS;
}

int MeterModbus::close() {
	_mbconn.reset();
	return SUCCESS;
}

ssize_t MeterModbus::read(std::vector<Reading> &rds, size_t n) {
	rds.clear();
	try {
		for (auto&& i : _devices) {
			slaveid_t slave = i.first;
			RegisterMap::Ptr regmap = i.second;
			regmap->read(rds, _mbconn, slave);
		}
		return rds.size();
	} catch (ModbusException &e) {
		print(log_error, "Modbus read error: %s. Re-connecting.", name().c_str(), e.what());
		_mbconn->close();
		_mbconn->connect();
		return 0;
	}
}

void ModbusConnection::connect() {
	int ret = modbus_connect(_ctx);
	if (ret < 0) {
		throw ModbusException("connecting modbus");
	}

}

void ModbusConnection::close() {
	modbus_close(_ctx);
}

ModbusConnection::~ModbusConnection() {
	if (_ctx) {
		modbus_close(_ctx);
		modbus_free(_ctx);
		_ctx = NULL;
	}
}

void ModbusConnection::read_registers(int addr, int nb, uint16_t *dest, unsigned slave) {
	modbus_set_slave(_ctx, slave);
	int ret = modbus_read_registers(_ctx, addr, nb, dest);
	if (ret < 0)
		throw(ModbusException("modbus_read_registers failed."));
}

void ModbusConnection::debug(bool enable) {
	modbus_set_debug(_ctx, TRUE);
}
ModbusRTUConnection::ModbusRTUConnection(const std::string &device, int baud) {
	_ctx = modbus_new_rtu(device.c_str(), baud, 'N', 8, 1);
	if (_ctx == NULL)
		throw ModbusException("creating new rtu");
}

ModbusTCPConnection::ModbusTCPConnection(const std::string &ip, int port) {
	_ctx = modbus_new_tcp(ip.c_str(), port);
	if (_ctx == NULL)
		throw ModbusException("creating new tcp");
}


std::map<std::string, RegisterMap::Ptr (*)()> RegisterMap::maps = {
		{ "ohmpilot", createMap<OpRegisterMap> },
		{ "imemeter", createMap<IMEmeterRegisterMap> }
};

void OpRegisterMap::read(std::vector<Reading>& rds, ModbusConnection::Ptr conn, unsigned id) {
	uint16_t regs[10];

	conn->read_registers(40799, 10, regs, id);
	rds.push_back(Reading(regs[9] / 10.0, new StringIdentifier("T")));
	rds.push_back(Reading(regs[2], new StringIdentifier("P")));
	rds.push_back(Reading(regs[8], new StringIdentifier("E")));
}

void IMEmeterRegisterMap::read(std::vector<Reading>& rds, ModbusConnection::Ptr conn, unsigned id) {
	const int reg_offset = 4096;
	const int reg_len = 59;
	uint16_t regs[reg_len];
	int value;

	conn->read_registers(reg_offset, reg_len, regs, id);

	value = MODBUS_GET_INT32_FROM_INT16(regs, 4116 - reg_offset);
	rds.push_back(Reading(value * 0.01, new StringIdentifier("Current Power")));

	value = MODBUS_GET_INT32_FROM_INT16(regs, 4128 - reg_offset);
	rds.push_back(Reading(value, new StringIdentifier("TotalExpWh")));

	value = MODBUS_GET_INT32_FROM_INT16(regs, 4124 - reg_offset);
	rds.push_back(Reading(value, new StringIdentifier("TotalImpWh")));

}

