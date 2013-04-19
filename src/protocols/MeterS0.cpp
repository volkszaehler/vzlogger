/**
 * S0 Hutschienenzähler directly connected to an rs232 port
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
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

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#include "protocols/MeterS0.hpp"
#include "Options.hpp"
#include <VZException.hpp>

MeterS0::MeterS0(std::list<Option> options)
		: Protocol("s0")
{
	OptionList optlist;

	try {
		_device = optlist.lookup_string(options, "device");
	} catch( vz::VZException &e ) {
		print(log_error, "Missing device or invalid type", "");
		throw;
	}

	try {
		_resolution = optlist.lookup_int(options, "resolution");
	} catch( vz::OptionNotFoundException &e ) {
		_resolution = 1000;
	} catch( vz::VZException &e ) {
		print(log_error, "Failed to parse resolution", "");
		throw;
	}
	if(_resolution < 1) throw vz::VZException("Resolution must be greater than 0.");
}

MeterS0::~MeterS0() {
	//free((void*)_device);
}

int MeterS0::open() {

	/* open port */
	int fd = ::open(_device, O_RDWR | O_NOCTTY); 

	if (fd < 0) {
		print(log_error, "open(%s): %s", "", _device, strerror(errno));
		return ERR;
	}

	/* save current port settings */
	tcgetattr(fd, &_old_tio);

	/* configure port */
	struct termios tio;
	memset(&tio, 0, sizeof(struct termios));

	tio.c_cflag = B300 | CS8 | CLOCAL | CREAD;
	tio.c_iflag = IGNPAR;
	tio.c_oflag = 0;
	tio.c_lflag = 0;
	tio.c_cc[VMIN]=1;
	tio.c_cc[VTIME]=0;

	tcflush(fd, TCIFLUSH);

	/* apply configuration */
	tcsetattr(fd, TCSANOW, &tio);
	_fd = fd;

	return SUCCESS;
}

int MeterS0::close() {

	tcsetattr(_fd, TCSANOW, &_old_tio); /* reset serial port */

	return ::close(_fd); /* close serial port */
}

ssize_t MeterS0::read(std::vector<Reading> &rds, size_t n) {
	char buf[8];

	/* check that we can store the data */
	if(n<1) return 0;

	/* clear input buffer */
	tcflush(_fd, TCIOFLUSH);

	/* blocking until one character/pulse is read */
	if( ::read(_fd, buf, 8) < 1) return 0;

	rds[0].identifier(new NilIdentifier());
	rds[0].value(1);
	rds[0].time();

	/* wait some ms for debouncing */
	usleep(30000);

	return 1;
}

