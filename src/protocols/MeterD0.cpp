/**
 * Plaintext protocol according to DIN EN 62056-21
 *
 * This protocol uses OBIS codes to identify the readout data
 *
 * This is our example protocol. Use this skeleton to add your own
 * protocols and meters.
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
 * @author Mathias Dalheimer <md@gonium.net>
 */
/*
 * This file is part of volkzaehler.org
 *
 * volkzaehler.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * volkzaehler.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with volkszaehler.org. If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// socket
#include <netdb.h>
#include <sys/socket.h>

#include "protocols/MeterD0.hpp"
#include <VZException.hpp>

#include "Obis.hpp"

#define STX 0x02

MeterD0::MeterD0(std::list<Option> &options)
	: Protocol("d0"), _host(""), _device(""), _auto_ack(false), _wait_sync_end(false),
	  _read_timeout_s(10), _baudrate_change_delay_ms(0), _reaction_time_ms(200) // default to 200ms
	  ,
	  _dump_fd(0), _old_mode(NONE), _dump_pos(0) {
	OptionList optlist;

	// connection
	try {
		_host = optlist.lookup_string(options, "host");
	} catch (vz::OptionNotFoundException &e) {
		try {
			_device = optlist.lookup_string(options, "device");
			if (!_device.length())
				throw vz::VZException("device without length");
		} catch (vz::VZException &e) {
			print(log_alert, "Missing device or host", name().c_str());
			throw;
		}
	} catch (vz::VZException &e) {
		print(log_alert, "Missing device or host", name().c_str());
		throw;
	}

	try {
		_dump_file = optlist.lookup_string(options, "dump_file");
	} catch (vz::OptionNotFoundException &e) {
		// default keep disabled
	}

	try {
		std::string hex;
		hex = optlist.lookup_string(options, "pullseq");
		int n = hex.size();
		int i;
		for (i = 0; i < n; i = i + 2) {
			char hs[3];
			hs[2] = 0;
			strncpy(hs, hex.c_str() + i, 2);
			char hx[2];
			hx[0] = strtol(hs, NULL, 16);
			_pull.append(hx, 1);
		}
		print(log_debug, "pullseq len:%d found", name().c_str(), _pull.size());
	} catch (vz::OptionNotFoundException &e) {
		// using default value if not specified
		_pull = "";
	}
	try {
		std::string hex;
		hex = optlist.lookup_string(options, "ackseq");
		if (!hex.compare("auto")) {
			_auto_ack = true;
			print(log_debug, "using autoack", name().c_str());
		} else {
			int n = hex.size();
			int i;
			for (i = 0; i < n; i = i + 2) {
				char hs[3];
				strncpy(hs, hex.c_str() + i, 2);
				char hx[2];
				hx[0] = strtol(hs, NULL, 16);
				_ack.append(hx, 1);
			}
			print(log_debug, "ackseq len:%d found %s, %x", name().c_str(), _ack.size(),
				  _ack.c_str(), _ack.c_str()[0]);
		}
	} catch (vz::OptionNotFoundException &e) {
		// using default value if not specified
		_ack = "";
	}

	// baudrate
	int baudrate = 9600; // default to avoid compiler warning
	try {
		baudrate = optlist.lookup_int(options, "baudrate");
		// find constant for termios structure
		switch (baudrate) {
		case 50:
			_baudrate = B50;
			break;
		case 75:
			_baudrate = B75;
			break;
		case 110:
			_baudrate = B110;
			break;
		case 134:
			_baudrate = B134;
			break;
		case 150:
			_baudrate = B150;
			break;
		case 200:
			_baudrate = B200;
			break;
		case 300:
			_baudrate = B300;
			break;
		case 600:
			_baudrate = B600;
			break;
		case 1200:
			_baudrate = B1200;
			break;
		case 1800:
			_baudrate = B1800;
			break;
		case 2400:
			_baudrate = B2400;
			break;
		case 4800:
			_baudrate = B4800;
			break;
		case 9600:
			_baudrate = B9600;
			break;
		case 19200:
			_baudrate = B19200;
			break;
		case 38400:
			_baudrate = B38400;
			break;
		case 57600:
			_baudrate = B57600;
			break;
		case 115200:
			_baudrate = B115200;
			break;
		case 230400:
			_baudrate = B230400;
			break;
		default:
			print(log_alert, "RW:Invalid baudrate: %i", name().c_str(), baudrate);
			throw vz::VZException("Invalid baudrate");
		}
	} catch (vz::OptionNotFoundException &e) {
		// using default value if not specified
		_baudrate = B9600;
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse the baudrate", name().c_str());
		throw;
	}
	try {
		baudrate = optlist.lookup_int(options, "baudrate_read");
		// find constant for termios structure
		switch (baudrate) {
		case 50:
			_baudrate_read = B50;
			break;
		case 75:
			_baudrate_read = B75;
			break;
		case 110:
			_baudrate_read = B110;
			break;
		case 134:
			_baudrate_read = B134;
			break;
		case 150:
			_baudrate_read = B150;
			break;
		case 200:
			_baudrate_read = B200;
			break;
		case 300:
			_baudrate_read = B300;
			break;
		case 600:
			_baudrate_read = B600;
			break;
		case 1200:
			_baudrate_read = B1200;
			break;
		case 1800:
			_baudrate_read = B1800;
			break;
		case 2400:
			_baudrate_read = B2400;
			break;
		case 4800:
			_baudrate_read = B4800;
			break;
		case 9600:
			_baudrate_read = B9600;
			break;
		case 19200:
			_baudrate_read = B19200;
			break;
		case 38400:
			_baudrate_read = B38400;
			break;
		case 57600:
			_baudrate_read = B57600;
			break;
		case 115200:
			_baudrate_read = B115200;
			break;
		case 230400:
			_baudrate_read = B230400;
			break;
		default:
			print(log_alert, "RW:Invalid baudrate_read: %i", name().c_str(), baudrate);
			throw vz::VZException("Invalid baudrate");
		}
	} catch (vz::OptionNotFoundException &e) {
		// using default value if not specified
		_baudrate_read = _baudrate;
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse the baudrate_read", name().c_str());
		throw;
	}

	_parity = parity_7e1;
	try {
		const char *parity = optlist.lookup_string(options, "parity");
		// find constant for termios structure
		if (strcasecmp(parity, "8n1") == 0) {
			_parity = parity_8n1;
		} else if (strcasecmp(parity, "7n1") == 0) {
			_parity = parity_7n1;
		} else if (strcasecmp(parity, "7e1") == 0) {
			_parity = parity_7e1;
		} else if (strcasecmp(parity, "7o1") == 0) {
			_parity = parity_7o1;
		} else {
			throw vz::VZException("Invalid parity");
		}
	} catch (vz::OptionNotFoundException &e) {
		// using default value if not specified
		_parity = parity_7e1;
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse the parity", name().c_str());
		throw;
	}

	try {
		const char *waitsync = optlist.lookup_string(options, "wait_sync");
		if (strcasecmp(waitsync, "end") == 0) {
			_wait_sync_end = true;
		} else if (strcasecmp(waitsync, "off") == 0) {
			_wait_sync_end = false;
		} else {
			throw vz::VZException("Invalid wait_sync");
		}
	} catch (vz::OptionNotFoundException &e) {
		// no fault. default is off
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse wait_sync", name().c_str());
		throw;
	}

	try {
		_read_timeout_s = optlist.lookup_int(options, "read_timeout");
	} catch (vz::OptionNotFoundException &e) {
		// use default: 10s from constructor
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse read_timeout", name().c_str());
		throw;
	}

	try {
		_baudrate_change_delay_ms = optlist.lookup_int(options, "baudrate_change_delay");
	} catch (vz::OptionNotFoundException &e) {
		// use default: (disabled) from constructor
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse baudrate_change_delay", name().c_str());
		throw;
	}
}

MeterD0::~MeterD0() {
	if (_dump_fd)
		(void)fclose(_dump_fd);
}

int MeterD0::open() {

	if (_dump_file.length()) {
		if (_dump_fd)
			(void)fclose(_dump_fd);
		_dump_fd = fopen(_dump_file.c_str(), "a");
		if (!_dump_fd)
			print(log_alert, "Failed to open dump_file %s (%d)", name().c_str(), _dump_file.c_str(),
				  errno);
		dump_file(CTRL, "opened");
	}

	if (_device.length() > 0) {
		_fd = _openDevice(&_oldtio, _baudrate);
	} else if (_host.length() > 0) {
		char *addr = strdup(host());
		const char *node = strsep(&addr, ":");
		const char *service = strsep(&addr, ":");

		_fd = _openSocket(node, service);
	}

	return (_fd < 0) ? ERR : SUCCESS;
}

int MeterD0::close() {
	dump_file(CTRL, "closed\n"); // here we add a \n as this will be the last data
	if (_dump_fd) {
		(void)fclose(_dump_fd);
		_dump_fd = 0;
	}
	return ::close(_fd);
}

ssize_t MeterD0::read(std::vector<Reading> &rds, size_t max_readings) {

	enum {
		START,
		VENDOR,
		BAUDRATE,
		IDENTIFICATION,
		ACK,
		START_LINE,
		OBIS_CODE,
		VALUE,
		UNIT,
		END_LINE,
		END
	} context;

	bool error_flag = false;
	const int VENDOR_LEN = 3;
	char vendor[VENDOR_LEN + 1] = {0}; // 3 upper case vendor + '\0' termination
	const int IDENTIFICATION_LEN = 16;
	char identification[IDENTIFICATION_LEN + 1] = {0}; // 16 meter specific + '\0' termination
	const int OBIS_LEN = 16;
	char obis_code[OBIS_LEN + 1]; /* A-B:C.D.E*F
							 fields A, B, E, F are optional
							 fields C & D are mandatory
							 A: energy type; 1: energy
							 B: channel number; 0: no channel specified
							 C: data items; 0-89 in COSEM context: IEC 62056-62, Clause D.1; 96:
							 General service entries 1:  Totel Active power+ 21: L1 Active power+
							 31: L1 Current
							 32: L1 Voltage
							 41: L2 Active power+
							 51: L2 Current
							 52: L2 Voltage
							 61: L3 Active power+
							 71: L3 Current
							 72: L3 Voltage
							 96.1.255: Metering point ID 256 (electricity related)
							 96.5.5: Meter started status flag
							 D: types
							 E: further processing or classification of quantities
							 F: storage of data
							 see DIN-EN-62056-61 */
	const int VALUE_LEN = 32;
	char value[VALUE_LEN + 1]; // value, i.e. the actual reading
	const int UNIT_LEN = 16;
	char unit[UNIT_LEN + 1]; // the unit of the value, e.g. kWh, V, ...

	char baudrate;       // 1 byte for
	char byte, lastbyte; // we parse our input byte wise
	int byte_iterator;
	char endseq[2 + 1]; // Endsequence ! not ?!
	size_t number_of_tuples;
	int bytes_read;
	time_t start_time, end_time;
	struct termios tio;
	int baudrate_connect, baudrate_read; // Baudrates for switching

	dump_file(CTRL, "read");

	baudrate_connect = _baudrate;
	baudrate_read = _baudrate_read;
	tcgetattr(_fd, &tio);

	if (_pull.size()) {
		dump_file(CTRL, "TCIOFLUSH and cfsetiospeed");
		tcflush(_fd, TCIOFLUSH);
		cfsetispeed(&tio, baudrate_connect);
		cfsetospeed(&tio, baudrate_connect);
		// apply new configuration
		tcsetattr(_fd, TCSANOW, &tio);
		if (_baudrate_change_delay_ms)
			usleep(_baudrate_change_delay_ms *
				   1000); // give some time for baudrate change to be applied
		int wlen = write(_fd, _pull.c_str(), _pull.size());
		dump_file(DUMP_OUT, _pull.c_str(), wlen > 0 ? wlen : 0);
		print(log_debug, "sending pullsequenz send (len:%d is:%d).", name().c_str(), _pull.size(),
			  wlen);
	}

	time(&start_time);

	byte_iterator = number_of_tuples = baudrate = 0;
	byte = lastbyte = 0;
	context = START; // start with context START

	if (_wait_sync_end) {
		/* wait once for the sync pattern ("!") at the end of a regular D0 message.
		   This is intended for D0 meters that start sending data automatically
		   (e.g. Hager EHZ361).
		*/
		int skipped = 0;
		while (_wait_sync_end && ::read(_fd, &byte, 1)) {
			dump_file(byte);
			if (byte == '!') {
				_wait_sync_end = false;
				print(log_debug, "found wait_sync_end. skipped %d bytes.", name().c_str(), skipped);
			} else {
				skipped++;
				if (skipped > D0_BUFFER_LENGTH) {
					_wait_sync_end = false;
					print(log_error,
						  "stopped searching for wait_sync_end after %d bytes without success!",
						  name().c_str(), skipped);
				}
			}
		}
	}

	while (1) {
		// check for timeout
		time(&end_time);
		if (difftime(end_time, start_time) > _read_timeout_s) {
			print(log_error, "nothing received for more than %d seconds", name().c_str(),
				  _read_timeout_s);
			dump_file(CTRL, "timeout!");
			break;
		}

		// now read a single byte
		bytes_read = ::read(_fd, &byte, 1);
		if (bytes_read == 0 || (bytes_read == -1 && errno == EAGAIN)) {
			// wait 5ms and read again
			usleep(5000);
			continue;
		} else if (bytes_read == -1) {
			print(log_error, "error reading a byte (%d)", name().c_str(), errno);
			break;
		}
		dump_file(byte);

		// reset timeout if we are making progress
		if (context != START) {
			time(&start_time);
		}

		lastbyte = byte;
		if ((byte == '/') && (byte_iterator == 0)) {
			context = VENDOR; // Slash can also be in OBIS String of TD-3511 meter
		} else if ((byte == '?') || (byte == '!')) {
			if (context != END) {
				context = END; // "!" is the identifier for the END
				byte_iterator = 0;
			}
		}

		switch (context) {
		case START:            // strip the initial "/"
			if (byte == '/') { // if ((byte != '\r') &&  (byte != '\n')) { 	// allow extra new line
							   // at the start
				byte_iterator = number_of_tuples = 0; // start
				context = VENDOR;                     // set new context: START -> VENDOR
			} // else ignore the other chars. -> Wait for / (!? is checked above already)
			break;

		case VENDOR: // VENDOR has 3 Bytes
			if ((byte == '\r') || (byte == '\n') || (byte == '/')) {
				byte_iterator = number_of_tuples = 0;
				break;
			}

			if (!isalpha(byte))
				goto error;                    // Vendor ID needs to be alpha
			vendor[byte_iterator++] = byte;    // read next byte
			if (byte_iterator >= VENDOR_LEN) { // after 3rd byte
				// check for reaction time indicator: (3rd letter lower case)
				if (islower(vendor[2]))
					_reaction_time_ms = 20; // lower case indicates 20ms
				else
					_reaction_time_ms = 200; // upper case indicates 200ms

				vendor[byte_iterator] = '\0'; // termination
				byte_iterator = 0;            // reset byte counter
				context = BAUDRATE;           // set new context: VENDOR -> BAUDRATE
			}
			break;

		case BAUDRATE:       // BAUDRATE consists of 1 char only
			baudrate = byte; // with _auto_ack we could check here whether the baudrate changed and
							 // set _ack to ""
			byte_iterator = 0;
			context = IDENTIFICATION; // set new context: BAUDRATE -> IDENTIFICATION
			break;

		case IDENTIFICATION:                          // IDENTIFICATION has 16 bytes
			if ((byte == '\r') || (byte == '\n')) {   // line end
				identification[byte_iterator] = '\0'; // termination
				print(log_debug, "Pull answer (vendor=%s, baudrate=%c, identification=%s)",
					  name().c_str(), vendor, baudrate, identification);
				byte_iterator = 0;
				context = ACK; // set new context: IDENTIFICATION -> ACK (old: OBIS_CODE)
				// warning we send the ACK only after receiving of next char. This works only as the
				// ID is ended by \r \n
			} else {
				if (!isprint(byte)) {
					print(log_error, "====> binary character '%x'", name().c_str(), byte);
					// error_flag=true;
				} else {
					if (byte_iterator < IDENTIFICATION_LEN)
						identification[byte_iterator++] = byte;
					else
						print(log_error, "Too much data for identification (byte=0x%X)",
							  name().c_str(), byte);
				}
				// break;
			}
			break;

		case ACK:
			if (_auto_ack || _ack.size()) {
				// first delay according to min reaction time:
				usleep(_reaction_time_ms * 1000);

				if (!_ack.size()) {
					// calculate the ack seq based on IEC62056-21 mode C data readout:
					// assuming a meter doesn't change at runtime the baudrate
					_ack = "\x06\x30\x30\x30\x0d\x0a"; // 063030300d0a
					// now change based on baudrate:
					char c = 0;
					switch (baudrate) {
					case '1': // 600
						baudrate_read = B600;
						c = baudrate;
						break;
					case '2': // 1200
						baudrate_read = B1200;
						c = baudrate;
						break;
					case '3': // 2400
						baudrate_read = B2400;
						c = baudrate;
						break;
					case '4': // 4800
						baudrate_read = B4800;
						c = baudrate;
						break;
					case '5': // 9600
						baudrate_read = B9600;
						c = baudrate;
						break;
					case '6': // 19200
						baudrate_read = B19200;
						c = baudrate;
						break;
					case '0': // 300 nobreak;
					default:
						baudrate_read = 300; // don't set c
						break;
					}
					_baudrate_read = baudrate_read; // store in member variable as well overriding
													// the option parameter
					if (c != 0)
						_ack[2] = c;
				}

				// we have to send the ack with the old baudrate and change after successfull
				// transmission:
				int wlen = write(_fd, _ack.c_str(), _ack.size());
				dump_file(DUMP_OUT, _ack.c_str(), wlen);
				if (!_baudrate_change_delay_ms)
					tcdrain(_fd); // if no delay is defined we use tcdrain Wait until sent
				print(log_debug, "Sending ack sequence send (len:%d is:%d,%s).", name().c_str(),
					  _ack.size(), wlen, _ack.c_str());

				if (_baudrate_change_delay_ms)
					usleep(_baudrate_change_delay_ms * 1000);
				if (baudrate_read != baudrate_connect) {
					cfsetispeed(&tio, baudrate_read);
					cfsetospeed(
						&tio, baudrate_read); // we set this as well. might not be needed but
											  // adapters might not support different speed setups.
					tcsetattr(_fd, TCSADRAIN,
							  &tio); // TCSADRAIN should not be needed (TCSANOW might be sufficient)
					if (_baudrate_change_delay_ms)
						dump_file(CTRL, "usleep cfsetispeed");
					else
						dump_file(CTRL, "tcdrain cfsetispeed");
				}
			}
			context = OBIS_CODE;
			break;

		case START_LINE:
			break;

		case OBIS_CODE:
			print((log_level_t)(log_debug + 5), "DEBUG OBIS_CODE byte %c hex= %X ", name().c_str(),
				  byte, byte);
			if ((byte != '\n') && (byte != '\r') && (byte != 0x02)) { // exclude STX
				if (byte == '(') {
					obis_code[byte_iterator] = '\0';
					byte_iterator = 0;
					context = VALUE;
				} else {
					if (byte_iterator < OBIS_LEN)
						obis_code[byte_iterator++] = byte;
					else
						print(log_error, "Too much data for obis_code (byte=0x%X)", name().c_str(),
							  byte);
				}
			}
			break;

		case VALUE:
			print(((log_level_t)(log_debug + 5)), "DEBUG VALUE byte= %c hex= %x ", name().c_str(),
				  byte, byte);
			if ((byte == '*') || (byte == ')')) {
				value[byte_iterator] = '\0';
				byte_iterator = 0;

				if (byte == ')') {
					unit[0] = '\0';
					context = END_LINE;
				} else {
					context = UNIT;
				}
			} else {
				if (byte_iterator < VALUE_LEN)
					value[byte_iterator++] = byte;
				else
					print(log_error, "Too much data for value (byte=0x%X)", name().c_str(), byte);
			}
			break;

		case UNIT:
			if (byte == ')') {
				unit[byte_iterator] = '\0';
				byte_iterator = 0;
				context = END_LINE;
			} else {
				if (byte_iterator < UNIT_LEN)
					unit[byte_iterator++] = byte;
				else
					print(log_error, "Too much data for unit (byte=0x%X)", name().c_str(), byte);
			}
			break;

		case END_LINE:
			print(log_alert, "logical error in state machine. reached END_LINE", name().c_str());
			goto error; // this should never happen
			break;

		case END:
			// here we stay until we receive either:
			// a) ! as end indicator
			// b) ?! as pull seq indicator -> wait for VENDOR
			// c) how to handle ? with something else? -> ignore (so ??! will be accepted as b)
			// d) 0x0d 0x0a ? -> ignore, so stay in END state.
			// above is new! Previous versions ended on all but ? ("assuming !")

			if (byte == '!') {
				if (byte_iterator == 0) {
					// case a) ! as end ind.
					// fallthrough to return number of tuples below.
				} else {
					// can only be case b) ?!.
					if (endseq[0] == '?') {
						context = VENDOR;
						byte_iterator = 0;
						break;
					} else {
						error_flag = true; // state machine logic error!
						print(log_debug, "DEBUG END b2 byte: %x byte_it: %d ", name().c_str(), byte,
							  byte_iterator);
					}
				}
			} else if (byte == '?') {
				if (byte_iterator == 0) {
					// can be start of case b, store it
					endseq[byte_iterator++] = byte;
					break;
				} else {
					// we simply keep the state. so we accept ??! as well
					break;
				}
			} else if (byte == STX) {
				// some meter seem to send ? STX ... as start package. (e.g. AS1440)
				context = OBIS_CODE;
				byte_iterator = 0;
				break;
			} else if (byte == '/') { // go to vendor
				context = VENDOR;
				byte_iterator = 0;
				break;
			} else { // any other char than ! or ?:
				if (byte_iterator > 0)
					byte_iterator = 0; // reset ? reminder
				break; // but stay in this state and accept that char! (here we ended before!)
					   // TODO Think about a timeout here?
			}

			if (error_flag) {
				print(log_error, "reading binary values.", name().c_str());
				goto error;
			}

			print(log_debug,
				  "Read package with %llu tuples (vendor=%s, baudrate=%c, identification=%s)",
				  name().c_str(), (unsigned long long)number_of_tuples, vendor, baudrate,
				  identification);
			return number_of_tuples;
		} // end switch

		if (END_LINE == context) { // add the data already here (so after the closing bracket) but
								   // before any \r\n
			// free slots available and sane content?
			if ((number_of_tuples < max_readings) && (strlen(obis_code) > 0) &&
				(strlen(value) > 0)) {
				switch (obis_code[0]) { // let's check sanity of first char. we can't use isValid()
										// as here we get incomplete obis_codes as well (e.g. 1.8.0
										// -> 255-255:1.8.0)
				case '0':               // nobreak;
				case '1':               // nobreak;
				case '2':               // nobreak;
				case '3':               // nobreak;
				case '4':               // nobreak;
				case '5':               // nobreak;
				case '6':               // nobreak;
				case '7':               // nobreak;
				case '8':               // nobreak;
				case '9':               // nobreak;
				case 'C':               // nobreak;
				case 'F':
					print(log_debug, "Parsed reading (OBIS code=%s, value=%s, unit=%s)",
						  name().c_str(), obis_code, value, unit);
					rds[number_of_tuples].value(strtod(value, NULL));

					try {
						Obis obis(obis_code);
						ReadingIdentifier *rid(new ObisIdentifier(obis));
						rds[number_of_tuples].identifier(rid);
						rds[number_of_tuples].time();
						number_of_tuples++;
					} catch (vz::VZException &e) {
						print(log_alert, "Failed to parse obis code (%s)", name().c_str(),
							  obis_code);
					}
					break;
				case 'L': // nobreak; // L, P not supported yet
				case 'P': // nobreak;
				default:
					print(log_debug, "Ignored reading (OBIS code=%s, value=%s, unit=%s)",
						  name().c_str(), obis_code, value, unit);
					break;
				}
			}
			byte_iterator = 0;
			context = OBIS_CODE;
		}

	} // end while

	// Read terminated
	print(log_alert, "read timed out!, context: %i, bytes read: %i, last byte 0x%x", name().c_str(),
		  context, byte_iterator, lastbyte);
	return number_of_tuples; // in any case return the number of readings. there might be some valid
							 // ones.

error:
	print(log_alert, "Something unexpected happened: %s:%i!", name().c_str(), __FUNCTION__,
		  __LINE__);
	return number_of_tuples; // return number of good readings so far.
}

int MeterD0::_openSocket(const char *node, const char *service) {
	struct sockaddr_in sin;
	struct addrinfo *ais;
	int fd; // file descriptor
	int res;

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		print(log_alert, "socket(): %s", name().c_str(), strerror(errno));
		return ERR;
	}

	getaddrinfo(node, service, NULL, &ais);
	memcpy(&sin, ais->ai_addr, ais->ai_addrlen);
	freeaddrinfo(ais);

	res = connect(fd, (struct sockaddr *)&sin, sizeof(sin));
	if (res < 0) {
		print(log_alert, "connect(%s, %s): %s", name().c_str(), node, service, strerror(errno));
		return ERR;
	}

	return fd;
}

int MeterD0::_openDevice(struct termios *old_tio, speed_t baudrate) {
	struct termios tio;
	memset(&tio, 0, sizeof(struct termios));

	int fd = ::open(device(), O_RDWR);
	if (fd < 0) {
		print(log_alert, "open(%s): %s", name().c_str(), device(), strerror(errno));
		return ERR;
	}

	// get old configuration
	tcgetattr(fd, &tio);

	// backup old configuration to restore it when closing the meter connection
	memcpy(old_tio, &tio, sizeof(struct termios));
	/*
	initialize all control characters
	default values can be found in /usr/include/termios.h, and are given
	in the comments, but we don't need them here
	*/
	tio.c_cc[VINTR] = 0;    // Ctrl-c
	tio.c_cc[VQUIT] = 0;    // Ctrl-<backslash>
	tio.c_cc[VERASE] = 0;   // del
	tio.c_cc[VKILL] = 0;    // @
	tio.c_cc[VEOF] = 4;     // Ctrl-d
	tio.c_cc[VTIME] = 0;    // inter-character timer unused
	tio.c_cc[VMIN] = 1;     // blocking read until 1 character arrives
	tio.c_cc[VSWTC] = 0;    // '\0'
	tio.c_cc[VSTART] = 0;   // Ctrl-q
	tio.c_cc[VSTOP] = 0;    // Ctrl-s
	tio.c_cc[VSUSP] = 0;    // Ctrl-z
	tio.c_cc[VEOL] = 0;     // '\0'
	tio.c_cc[VREPRINT] = 0; // Ctrl-r
	tio.c_cc[VDISCARD] = 0; // Ctrl-u
	tio.c_cc[VWERASE] = 0;  // Ctrl-w
	tio.c_cc[VLNEXT] = 0;   // Ctrl-v
	tio.c_cc[VEOL2] = 0;    // '\0'

	tio.c_iflag &= ~(BRKINT | INLCR | IMAXBEL | IXOFF | IXON);
	tio.c_oflag &= ~(OPOST | ONLCR);
	tio.c_lflag &= ~(ISIG | ICANON | IEXTEN | ECHO);

	switch (_parity) {
	case parity_8n1:
		tio.c_cflag &= ~PARENB;
		tio.c_cflag &= ~PARODD;
		tio.c_cflag &= ~CSTOPB;
		tio.c_cflag &= ~CSIZE;
		tio.c_cflag |= CS8;
		break;
	case parity_7n1:
		tio.c_cflag &= ~PARENB;
		tio.c_cflag &= ~PARODD;
		tio.c_cflag &= ~CSTOPB;
		tio.c_cflag &= ~CSIZE;
		tio.c_cflag |= CS7;
		break;
	case parity_7e1:
		tio.c_cflag &= ~CRTSCTS;
		tio.c_cflag |= PARENB;
		tio.c_cflag &= ~PARODD;
		tio.c_cflag &= ~CSTOPB;
		tio.c_cflag &= ~CSIZE;
		tio.c_cflag |= CS7;
		break;
	case parity_7o1:
		tio.c_cflag &= ~PARENB;
		tio.c_cflag |= PARODD;
		tio.c_cflag &= ~CSTOPB;
		tio.c_cflag &= ~CSIZE;
		tio.c_cflag |= CS7;
		break;
	}
	// Set return rules for read to prevent endless waiting
	tio.c_cc[VTIME] = 50; // inter-character timer  50*0.1
	tio.c_cc[VMIN] = 0;   // VTIME is timeout counter
	// now clean the modem line and activate the settings for the port

	tcflush(fd, TCIOFLUSH);
	// set baudrate
	cfsetispeed(&tio, baudrate);
	cfsetospeed(&tio, baudrate);

	// apply new configuration
	tcsetattr(fd, TCSANOW, &tio);

	return fd;
}

void MeterD0::dump_file(DUMP_MODE ctrl, const char *str) {
	if (_dump_fd)
		dump_file(ctrl, str, strlen(str));
}

// hexdump helper from linux kernel. (Linux/include/linkux/kernel.h)
// copyright see kernel.org (gpl2)
// only used as a speedup. can be easily replaced by sprintf...%02x...
// snip start

const char hex_asc[] = "0123456789abcdef";
#define hex_asc_lo(x) hex_asc[((x)&0x0f)]
#define hex_asc_hi(x) hex_asc[((x)&0xf0) >> 4]

static inline char *hex_byte_pack(char *buf, char byte) {
	*buf++ = hex_asc_hi(byte);
	*buf++ = hex_asc_lo(byte);
	return buf;
}

// snip from kernel end

// dump_file is not thread safe/reentrant capable! Not needed for meterD0
void MeterD0::dump_file(DUMP_MODE mode, const char *buf, size_t len) {
	if (!_dump_fd)
		return;
	const char *ctrl_start = "##### ";
	const char *ctrl_end = "\n";

	const char *dump_out_start = "<<<<< ";
	const char *dump_in_start = ">>>>> ";

	static char str_dump[3 * 16 + 2 + 18]; // we output 3 chars per byte plus whitespace plus the
										   // char itself, 16 chars max in one row

	if (_dump_pos == 0) {
		memset(str_dump, ' ', sizeof(str_dump) - 1);
		str_dump[sizeof(str_dump) - 1] =
			'\n'; // yes I know that the string is not zero terminated! (no problem here)
	}

	if (mode != _old_mode) {
		// dump out str?
		if (_dump_pos) {
			fwrite(str_dump, 1, sizeof(str_dump), _dump_fd);
			_dump_pos = 0;
			memset(str_dump, ' ', sizeof(str_dump) - 1);
		}

		fwrite(ctrl_end, 1, strlen(ctrl_end), _dump_fd);
		fflush(_dump_fd); // flush on each mode change
		const char *s = 0, *e = 0;
		switch (mode) {
		case CTRL:
			s = ctrl_start;
			e = 0;
			break;
		case DUMP_IN:
			s = dump_in_start;
			e = ctrl_end;
			break;
		case DUMP_OUT:
			s = dump_out_start;
			e = ctrl_end;
			break;
		default:
			s = ctrl_start;
			break;
		}
		fwrite(s, 1, strlen(s), _dump_fd);
		// output timestamp:
		struct timespec ts;
		if (!clock_gettime(CLOCK_MONOTONIC_RAW, &ts)) {
			static struct timespec ts_last = {0, 0};
			long delta = (ts.tv_sec * 1000L) + (ts.tv_nsec / 1000000L);
			delta -= (ts_last.tv_nsec / 1000000L);
			delta -= ts_last.tv_sec * 1000L;
			// delta /= 1000000L; // change into ms
			if (ts_last.tv_sec == 0)
				delta = 0;
			ts_last = ts;
			char tbuf[30];
			// time_t may be long or long long depending on architecture, casting to long long for
			// portability
			int l = snprintf(tbuf, sizeof(tbuf), "%2lld.%.9llds (%6ld ms) ",
							 (long long)ts.tv_sec % 100, (long long)ts.tv_nsec, delta);
			fwrite(tbuf, 1, l, _dump_fd);
		}
		if (e)
			fwrite(e, 1, strlen(e), _dump_fd);
	}
	switch (mode) {
	case CTRL: {
		fwrite(buf, 1, len, _dump_fd);
	} break;
	case DUMP_IN:
	case DUMP_OUT: {
		char *stro = &str_dump[0];
		const int strsize = sizeof(str_dump);

		// dump out in hex format: 16 bytes as xx yy ... zz	printable chars
		for (size_t i = 0; i < len; ++i) {
			(void)hex_byte_pack(stro + (_dump_pos * 3), buf[i]);
			stro[(3 * 16 + 2) + _dump_pos] = isprint(buf[i]) ? buf[i] : ' ';
			++_dump_pos;
			if (_dump_pos > 15) {
				fwrite(stro, 1, strsize, _dump_fd);
				_dump_pos = 0;
				memset(stro, ' ', strsize - 1);
			}
		}
	} break;
	default:
		break;
	}

	if (mode == CTRL)
		_old_mode = NONE;
	else // output CTRL messages on a single line each
		_old_mode = mode;
}
