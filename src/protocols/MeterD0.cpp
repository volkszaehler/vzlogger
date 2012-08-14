/**
 * Plaintext protocol according to DIN EN 62056-21
 *
 * This protocol uses OBIS codes to identify the readout data
 *
 * This is our example protocol. Use this skeleton to add your own
 * protocols and meters.
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011, The volkszaehler.org project
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

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>

/* socket */
#include <netdb.h>
#include <sys/socket.h>

#include "protocols/MeterD0.hpp"
#include <VZException.hpp>

#include "Obis.hpp"

MeterD0::MeterD0(std::list<Option> options) 
		: Protocol("d0")
		, _host("")
		, _device("")
{
	OptionList optlist;

	/* connection */
	try {
		_host = optlist.lookup_string(options, "host");
	} catch ( vz::OptionNotFoundException &e ) {
		try {
			_device = optlist.lookup_string(options, "device");
		} catch ( vz::VZException &e ){
			print(log_error, "Missing device or host", name().c_str());
			throw ;
		}
	} catch( vz::VZException &e ) {
		print(log_error, "Missing device or host", name().c_str());
		throw;
	}

	/* baudrate */
	int baudrate = 9600; /* default to avoid compiler warning */
	try {
		baudrate = optlist.lookup_int(options, "baudrate");
		/* find constant for termios structure */
		switch (baudrate) {
				case 1200: _baudrate = B1200; break;
				case 1800: _baudrate = B1800; break;
				case 2400: _baudrate = B2400; break;
				case 4800: _baudrate = B4800; break;
				case 9600: _baudrate = B9600; break;
				case 19200: _baudrate = B19200; break;
				case 38400: _baudrate = B38400; break;
				case 57600: _baudrate = B57600; break;
				case 115200: _baudrate = B115200; break;
				case 230400: _baudrate = B230400; break;
				default:
					print(log_error, "Invalid baudrate: %i", name().c_str(), baudrate);
					throw vz::VZException("Invalid baudrate");
		}
	} catch( vz::OptionNotFoundException &e ) {
		/* using default value if not specified */
		_baudrate = B9600;
	} catch( vz::VZException &e ) {
		print(log_error, "Failed to parse the baudrate", name().c_str());
		throw;
	}
}

MeterD0::~MeterD0() {
}

int MeterD0::open() {
	if (_device != "") {
		_fd = _openDevice(&_oldtio, _baudrate);
	}
	else if (_host != "") {
		char *addr = strdup(host());
		const char *node = strsep(&addr, ":");
		const char *service = strsep(&addr, ":");

		_fd = _openSocket(node, service);
	}

	return (_fd < 0) ? ERR : SUCCESS;
}

int MeterD0::close() {
	return ::close(_fd);
}

ssize_t MeterD0::read(std::vector<Reading>&rds, size_t max_readings) {

	enum { START, VENDOR, BAUDRATE, IDENTIFICATION, START_LINE, OBIS_CODE, VALUE, UNIT, END_LINE, END } context;

	bool error_flag = false;
	char vendor[3+1];		/* 3 upper case vendor + '\0' termination */
	char identification[16+1];	/* 16 meter specific + '\0' termination */
	char obis_code[16+1];		/* A-B:C.D.E*F
														 fields A, B, E, F are optional
														 fields C & D are mandatory
														 A: energy type; 1: energy
														 B: channel number; 0: no channel specified
														 C: data items; 0-89 in COSEM context: IEC 62056-62, Clause D.1; 96: General service entries
														 1:  Totel Active power+
														 21: L1 Active power+
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
	char value[32+1];		/* value, i.e. the actual reading */
	char unit[16+1];		/* the unit of the value, e.g. kWh, V, ... */

	char baudrate;			/* 1 byte for */
	char byte;			/* we parse our input byte wise */
	int byte_iterator; 
	size_t number_of_tuples;

	byte_iterator =  number_of_tuples = baudrate = 0;

	context = START;				/* start with context START */

	while (::read(_fd, &byte, 1)) {
		if (byte == '/') context = START; 	/* reset to START if "/" reoccurs */
		else if (byte == '!') context = END;	/* "!" is the identifier for the END */
		switch (context) {
				case START:			/* strip the initial "/" */
                    if  (byte != '\r' &&  byte != '\n') { /*allow extra new line at the start */
                      byte_iterator = number_of_tuples = 0;        /* start */
                      context = VENDOR;        /* set new context: START -> VENDOR */
                    }
					break;

				case VENDOR:			/* VENDOR has 3 Bytes */
					if (!isalpha(byte)) goto error; /* Vendor ID needs to be alpha */
					vendor[byte_iterator++] = byte;	/* read next byte */
					if (byte_iterator >= 3) {	/* stop after 3rd byte */
						vendor[byte_iterator] = '\0'; /* termination */
						byte_iterator = 0;	/* reset byte counter */

						context = BAUDRATE;	/* set new context: VENDOR -> BAUDRATE */
					} 
					break;

				case BAUDRATE:			/* BAUDRATE consists of 1 char only */
					baudrate = byte;	
					context = IDENTIFICATION;	/* set new context: BAUDRATE -> IDENTIFICATION */
					byte_iterator = 0;
					break;

				case IDENTIFICATION:		/* IDENTIFICATION has 16 bytes */
					if (byte == '\r' || byte == '\n') { /* detect line end */
						identification[byte_iterator] = '\0'; /* termination */
						context = OBIS_CODE;	/* set new context: IDENTIFICATION -> OBIS_CODE */
						byte_iterator = 0;
					}
					else {
						if(!isprint(byte)) {
							print(log_error, "====> binary character '%x'", name().c_str(), byte);
							//error_flag=true;
						}
						else {
							identification[byte_iterator++] = byte;
						}
					}
					break;

				case START_LINE:
					break;
				case OBIS_CODE:
					if ((byte != '\n') && (byte != '\r')) 
					{
						if (byte == '(') {
							obis_code[byte_iterator] = '\0';
							byte_iterator = 0;

							context = VALUE;
						}
						else obis_code[byte_iterator++] = byte;
					}
					break;

				case VALUE:
					if (byte == '*' || byte == ')') {
						value[byte_iterator] = '\0';
						byte_iterator = 0;

						if (byte == ')') {
							unit[0] = '\0';
							context =  END_LINE;
						}
						else {
							context = UNIT;
						}
					}
					else value[byte_iterator++] = byte;
					break;

				case UNIT:
					if (byte == ')') {
						unit[byte_iterator] = '\0';
						byte_iterator = 0;

						context = END_LINE;
					}
					else unit[byte_iterator++] = byte;
					break;

				case END_LINE:
					if (byte == '\r' || byte == '\n') {
						/* free slots available and sain content? */
						if ((number_of_tuples < max_readings) && (strlen(obis_code) > 0) && 
								(strlen(value) > 0)) {
							print(log_debug, "Parsed reading (OBIS code=%s, value=%s, unit=%s)", name().c_str(), obis_code, value, unit);
							rds[number_of_tuples].value(strtof(value, NULL));
							Obis obis(obis_code);
							ReadingIdentifier *rid(new ObisIdentifier(obis));
							rds[number_of_tuples].identifier(rid);
							rds[number_of_tuples].time();

							byte_iterator = 0;
							number_of_tuples++;

							context = OBIS_CODE;
						}
					}
					break;

				case END:
					if(error_flag) {
						print(log_error, "reading binary values.", name().c_str());
						goto error;
					}
					print(log_debug, "Read package with %i tuples (vendor=%s, baudrate=%c, identification=%s)",
								name().c_str(), number_of_tuples, vendor, baudrate, identification);
					return number_of_tuples;
		}
	}

	error:
	print(log_error, "Something unexpected happened: %s:%i!", name().c_str(), __FUNCTION__, __LINE__);
	return 0;
}

int MeterD0::_openSocket(const char *node, const char *service) {
	struct sockaddr_in sin;
	struct addrinfo *ais;
	int fd; /* file descriptor */
	int res;

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		print(log_error, "socket(): %s", name().c_str(), strerror(errno));
		return ERR;
	}

	getaddrinfo(node, service, NULL, &ais);
	memcpy(&sin, ais->ai_addr, ais->ai_addrlen);
	freeaddrinfo(ais);

	res = connect(fd, (struct sockaddr *) &sin, sizeof(sin));
	if (res < 0) {
		print(log_error, "connect(%s, %s): %s", name().c_str(), node, service, strerror(errno));
		return ERR;
	}

	return fd;
}

int MeterD0::_openDevice(struct termios *old_tio, speed_t baudrate) {
	struct termios tio;
	memset(&tio, 0, sizeof(struct termios));

	int fd = ::open(device(), O_RDWR);
	if (fd < 0) {
		print(log_error, "open(%s): %s", name().c_str(), device(), strerror(errno));
		return ERR;
	}

	/* get old configuration */
	tcgetattr(fd, &tio) ;

	/* backup old configuration to restore it when closing the meter connection */
	memcpy(old_tio, &tio, sizeof(struct termios));

	/*  set 7-N-1 */
	tio.c_iflag &= ~(BRKINT | INLCR | IMAXBEL);
	tio.c_oflag &= ~(OPOST | ONLCR);
	tio.c_lflag &= ~(ISIG | ICANON | IEXTEN | ECHO);
	tio.c_cflag &= ~(CSIZE | PARENB | PARODD | CSTOPB);
	tio.c_cflag |= (CS7 | PARENB);

	/* set baudrate */
	cfsetispeed(&tio, baudrate);
	cfsetospeed(&tio, baudrate);

	/* apply new configuration */
	tcsetattr(fd, TCSANOW, &tio);

	return fd;
}
