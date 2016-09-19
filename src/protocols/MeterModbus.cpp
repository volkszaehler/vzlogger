/**
 * @file MeterModbus.cpp
 * @brief
 *  Created on: Apr 25, 2016
 * @author Florian Achleitner <florian.achleitner@student.tugraz.at>
 */
#include "protocols/MeterModbus.hpp"

#include <sstream>
#include <chrono>
#include <thread>

ModbusException::ModbusException(const std::string& arg)
: std::runtime_error(arg), _errno(errno) {
	std::ostringstream oss;
	oss << std::runtime_error::what() << " errno " << _errno << ": " <<  modbus_strerror(_errno) << std::endl;
	_what = oss.str();
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

	json_object *slaves = OptionList::lookup_json_array(options, "devices");

//	std::string map = lookup_mandatory<const char *, OptionList::lookup_string>(options, "register_map", name());
	for(int i = 0; i < json_object_array_length(slaves); i++) {
		json_object *slave = json_object_array_get_idx(slaves, i);
		json_object *value;
		int id;
		std::string regmap;
		if (json_object_object_get_ex(slave, "id", &value) &&
				json_object_get_type(value) == json_type_int) {
			id = json_object_get_int(value);
		} else
			throw vz::OptionNotFoundException("'id' field not found in devices array.");
		if (json_object_object_get_ex(slave, "regmap", &value) &&
				json_object_get_type(value) == json_type_string) {
			regmap = json_object_get_string(value);
		} else
			throw vz::OptionNotFoundException("'regmap' field not found in devices array.");

		try {
			_devices[id] = RegisterMap::findMap(regmap);
			print(log_info, "Creating MeterModbus with id %d, RegisterMap %s.", name().c_str(), id, regmap.c_str());
		} catch (std::out_of_range &e) {
			throw(vz::OptionNotFoundException("Modbus register map invalid."));
		}
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
	try {
		_mbconn->connect();
	} catch (ModbusException &e) {
		print(log_error, "MeterModbus can not connect: %s. Will retry.", name().c_str(), e.what());
	}
	if (_libmodbus_debug)
		_mbconn->debug(true);
	return SUCCESS;
}

int MeterModbus::close() {
	_mbconn.reset();
	return SUCCESS;
}

ssize_t MeterModbus::read(std::vector<Reading> &rds, size_t n) {
	bool one_success = false;
	rds.clear();
	for (auto&& i : _devices) {
		slaveid_t slave = i.first;
		RegisterMap::Ptr regmap = i.second;
		try {
			regmap->read(rds, _mbconn, slave);
			one_success = true;
		} catch (ModbusException &e) {
			print(log_error, "Modbus read error slave %d: %s. Skip.", name().c_str(), slave, e.what());
		}
	}
	if (!one_success) {
		print(log_error, "Modbus read unsuccessful, re-connecting.", name().c_str());
		try {
			_mbconn->close();
			_mbconn->connect();
		} catch (ModbusException &e) {
			print(log_error, "Reconnect failed: %s. Will retry.", name().c_str(), e.what());
		}
	}
	return rds.size();
}

void ModbusConnection::connect() {
	if (!_ctx)
		open();
	int ret = modbus_connect(_ctx);
	if (ret < 0) {
		throw ModbusException("connecting modbus");
	}
	_connected = true;

}

void ModbusConnection::close() {
	if (_ctx) {
		if (_connected) {
			modbus_close(_ctx);
			_connected = false;
		}
		modbus_free(_ctx);
		_ctx = NULL;
	}
}

ModbusConnection::~ModbusConnection() {
	close();
}

void ModbusConnection::read_registers(int addr, int nb, uint16_t *dest, unsigned slave) {
	if (!_ctx || !_connected)
		throw(ModbusException("modbus_read_registers not connected."));
	modbus_set_slave(_ctx, slave);
	int ret = modbus_read_registers(_ctx, addr, nb, dest);
	if (ret < 0)
		throw(ModbusException("modbus_read_registers failed."));
}

void ModbusConnection::debug(bool enable) {
	modbus_set_debug(_ctx, TRUE);
}
void ModbusRTUConnection::open() {
	close();
	_ctx = modbus_new_rtu(_device.c_str(), _baud, 'N', 8, 1);
	if (_ctx == NULL)
		throw ModbusException("creating new rtu");
}

void ModbusTCPConnection::open() {
	close();
	_ctx = modbus_new_tcp(_ip.c_str(), _port);
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
	rds.push_back(Reading(regs[9] / 10.0, new ModbusReadingIdentifier(id, "T")));
	rds.push_back(Reading(regs[2], new ModbusReadingIdentifier(id, "P")));
	rds.push_back(Reading(regs[8], new ModbusReadingIdentifier(id, "E")));
}

void IMEmeterRegisterMap::read(std::vector<Reading>& rds, ModbusConnection::Ptr conn, unsigned id) {
	const int reg_offset = 4096;
	const int reg_len = 59;
	uint16_t regs[reg_len];
	int value;

	// This device requires some time of silence on the bus, otherwise it doesn't respond.
	std::this_thread::sleep_for(std::chrono::milliseconds(20));

	conn->read_registers(reg_offset, reg_len, regs, id);

	value = MODBUS_GET_INT32_FROM_INT16(regs, 4116 - reg_offset);
	rds.push_back(Reading(value * 0.01, new ModbusReadingIdentifier(id, "CurrentPowerW")));

	value = MODBUS_GET_INT32_FROM_INT16(regs, 4128 - reg_offset);
	rds.push_back(Reading(value, new ModbusReadingIdentifier(id, "TotalExpWh")));

	value = MODBUS_GET_INT32_FROM_INT16(regs, 4124 - reg_offset);
	rds.push_back(Reading(value, new ModbusReadingIdentifier(id, "TotalImpWh")));

}

void ModbusReadingIdentifier::parse(const std::string& s)
{
	std::istringstream ss(s);
	char c;
	ss >> _slaveid >> c >> _name;
}

ModbusReadingIdentifier::ModbusReadingIdentifier(const std::string& conf)
: _slaveid(0)
{
	parse(conf);
	print(log_info, "ModbusReadingIdentifier, slave %d name %s", "modbus", _slaveid, _name.c_str());
}

bool ModbusReadingIdentifier::operator ==(const ReadingIdentifier& cmp) const
{
	const ModbusReadingIdentifier *ri = dynamic_cast<const ModbusReadingIdentifier*>(&cmp);
	return ri &&
			ri->_name == _name &&
			ri->_slaveid == _slaveid;
}

const std::string ModbusReadingIdentifier::toString() {
	std::ostringstream ss;
	ss << _slaveid << ":" << _name;
	return ss.str();
}

size_t ModbusReadingIdentifier::unparse(char *buffer, size_t n) {
	size_t l = toString().copy(buffer, n);
	buffer[n-1] = '\0';
	return l;
}
