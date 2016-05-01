/**
 * @file MeterModbus.cpp
 * @brief
 *  Created on: Apr 25, 2016
 * @author Florian Achleitner <florian.achleitner@student.tugraz.at>
 */
#include "protocols/MeterModbus.hpp"

MeterModbus::MeterModbus(const std::list<Option> &options) :
Protocol("modbus"),
_baudrate(9600),
_libmodbus_debug(false),
_slave(1)

{
	OptionList optlist;
	try {
		_device = optlist.lookup_string(options, "device");
	} catch (vz::VZException &e) {
		print(log_error, "Missing path or invalid type", name().c_str());
		throw;
	}

	// optional baudrate, default to 9600:
	try {
		_baudrate = optlist.lookup_int(options, "baudrate");
	} catch (vz::VZException &e) {
		// keep default
	}

	try {
		_slave = optlist.lookup_int(options, "slave");
	} catch (vz::VZException &e) {
		// keep default
	}

	// optional mbus_debug, default to false
	try {
		_libmodbus_debug = optlist.lookup_bool(options, "libmodbus_debug");
	} catch (vz::VZException &e) {
		// keep default
	}

}

MeterModbus::~MeterModbus()
{
	// TODO Auto-generated destructor stub
}

int MeterModbus::open() {
	_mbconn.reset(new ModbusConnection(_device, _baudrate, _slave));
	if (_libmodbus_debug)
		modbus_set_debug(_mbconn->getctx(), TRUE);
	return SUCCESS;
}

int MeterModbus::close() {
	_mbconn.reset();
	return SUCCESS;
}

ssize_t MeterModbus::read(std::vector<Reading> &rds, size_t n) {
	int ret;
	uint16_t regs[2];
	int value, i;
	unsigned rs = 0;

	while (rs < n) {
		ret = -3;
		for (i = 0; ret < 0 && i < 10; i++) {
			ret = modbus_read_registers(_mbconn->getctx(), 4116, 2, regs);
			if (ret < 0) {
				print(log_error, "modbus_read_registers %d: %s", "Modbus", ret, modbus_strerror(errno));
			}
		}
		value = MODBUS_GET_INT32_FROM_INT16(regs, 0);
		rds[rs].value(value * 0.01);
		rds[rs].identifier(new StringIdentifier("Current Power"));
		rds[rs].time();
		rs++;
		break;
	}
	return rs;

}

ModbusConnection::ModbusConnection(const char *device, int baud, int slave)
: ctx(NULL) {
	ctx = modbus_new_rtu(device, baud, 'N', 8, 1);
	if (ctx == NULL)
		throw ModbusException("creating new rtu");
	int ret = modbus_connect(ctx);
	if (ret < 0) {
		throw ModbusException("connecting new rtu");
	}

	modbus_set_slave(ctx, slave);
}
ModbusConnection::~ModbusConnection()
{
	if (ctx) {
		modbus_close(ctx);
		modbus_free(ctx);
		ctx = NULL;
	}
}
