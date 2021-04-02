/**
 * crappy chinese hall-effect DC meter with serial output
 *  (no name, the only markings on the board are
 *         "ACCU V1.0  2018.3.10"  )
 *                       /   \
 *                      /     \
 *                     /   / \ \
 *                    /   /   \ \
 *                   /   /     \ \
 *                  /   /       \ \
 *                 /   /         \ \
 *                /    |\         \ \
 *               /     \ \         \ \
 *              /       \ \         \ \
 *             /         \ \         \ \
 *            /           \ \        /| \
 *           /    /\       \ \      / |  \
 *          /    / /        \ \    / / 1 /
 *         /    / / /\       \ \  / / V /
 *        /    / / / /        \ \/ /   /
 *       /   U3\/ / /    /\    \ |/ U /
 *       \ P1    / /    /  \-\  \/ C /
 *        \  | U2\/    /    \ \   C /
 *         \ #|        \     \ \ A /
 *          \ #|    U1 /\     \ \ /
 *           \ #|      \ \    /  \
 *            \ #|      \ \  / \  |
 *             \ #|      \ \/ / | |
 *              \ #|   /\ \  \  | /
 *               \ #  /  \ \  | |/
 *                \   \R5/ /| |
 *                 \ / \/ / | /
 *                  \ /  /  |/
 *                   \  /
 *                    \/
 * (code based on MeterD0)
 *
 * @package vzlogger
 * @copyright Copyright (c) 2021, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
 * @author Mathias Dalheimer <md@gonium.net>
 * @author Thorben T. <r00t@constancy.org>
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

#include "protocols/MeterChinaDC.hpp"
#include <VZException.hpp>

MeterChinaDC::MeterChinaDC(std::list<Option> &options)
	: Protocol("chinadc"), _device(""), _read_timeout_s(10) {
	OptionList optlist;

	// connection
	try {
		_device = optlist.lookup_string(options, "device");
		if (!_device.length())
			throw vz::VZException("empty devicename");
	} catch (vz::VZException &e) {
		print(log_alert, "Missing device", name().c_str());
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
}

MeterChinaDC::~MeterChinaDC() {}

int MeterChinaDC::open() {
	_fd = _openDevice(&_oldtio);
	return (_fd < 0) ? ERR : SUCCESS;
}

int MeterChinaDC::close() { return ::close(_fd); }

ssize_t MeterChinaDC::read(std::vector<Reading> &rds, size_t max_readings) {

	enum { MSB, LSB } context;

	char byte, msb;
	int read = 0, bogus = 0, desyncs = 0;
	int bytes_read;
	time_t start_time, end_time;

	time(&start_time);

	msb = 0;
	context = MSB;

	while (1) {
		// check for timeout
		time(&end_time);
		if (difftime(end_time, start_time) > _read_timeout_s) {
			print(log_error, "nothing received for more than %d seconds", name().c_str(),
				  _read_timeout_s);
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

		read++;

		if ((byte & 0xf0) > 0xb0 || (byte & 0x0f) > 0x09) {
			// bogus in any case, meter should never send this
			bogus++;
			continue;
		}

		// time(&start_time); // reset timeout if we are making progress

		switch (context) {
		case LSB:
			if ((byte & 0xf0) <= 0x90) {
				// got reading
				// print(log_debug, "sign: %hhi", name().c_str(), (((msb&0xf0)==0xb0)?-1:1));
				// print(log_debug, "tens: %hhi", name().c_str(), (msb&0x0f));
				// print(log_debug, "ones: %hhi", name().c_str(), (byte&0xf0)>>4 );
				// print(log_debug, "frac: %hhi", name().c_str(), (byte&0x0f));
				float value =
					(10.0 * (msb & 0x0f) + 1.0 * ((byte & 0xf0) >> 4) + 0.1 * (byte & 0x0f)) *
					(((msb & 0xf0) == 0xb0) ? -1 : 1);
				print(log_debug, "got reading %hhx%hhx=%f", name().c_str(), msb, byte, value);
				rds[0].value(value);
				rds[0].identifier(new StringIdentifier("current"));
				rds[0].time();
				return 1;
			}
			desyncs++;
			// fall through
		case MSB:
			if ((byte & 0xf0) != 0xa0 && (byte & 0xf0) != 0xb0) {
				desyncs++;
				continue;
			}
			msb = byte;
			context = LSB;
		}

	} // while

	// Read terminated
	print(log_alert, "read timed out!, bogus bytes read: %i, desyncs: %i", name().c_str(), bogus,
		  desyncs);
	return 0; // no tuples read
}

int MeterChinaDC::_openDevice(struct termios *old_tio) {
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

	// 8N1
	tio.c_cflag &= ~PARENB;
	tio.c_cflag &= ~PARODD;
	tio.c_cflag &= ~CSTOPB;
	tio.c_cflag &= ~CSIZE;
	tio.c_cflag |= CS8;

	// Set return rules for read to prevent endless waiting
	tio.c_cc[VTIME] = 50; // inter-character timer  50*0.1
	tio.c_cc[VMIN] = 0;   // VTIME is timeout counter
	// now clean the modem line and activate the settings for the port

	tcflush(fd, TCIOFLUSH);
	// set baudrate
	cfsetispeed(&tio, B9600);
	cfsetospeed(&tio, B9600);

	// apply new configuration
	tcsetattr(fd, TCSANOW, &tio);

	return fd;
}
