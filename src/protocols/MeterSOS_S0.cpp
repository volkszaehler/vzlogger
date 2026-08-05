/**
 * Plaintext protocol of S0 Pulse meter devices
 *  --> https://www.sossolutions.nl/5-kanaals-s0-pulse-meter-op-usb
 *
 * The device is a USB s0 logger device.
 *
 * The protocol is defined as follows:
 *       ‘ID:x:I:y:M1:a:b:M2:c:d:M3:e:f:M4:g:h:M5:i:j’’
 *        - x = ID des S0 pulse meter (unique)
 * 	      - y = number of seconds since last message (default: 10 second)
 *        - M1,M2,M3,M4,M5 = identifier name for corresponding S0 port on device
 *        - a,c,e,g,i = number of pulses since last message
 *        - b,d,f,h,j = total number of pulses since start of device
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Lurchi70 <https://github.com/Lurchi70/vzlogger>
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

#include "threads.h"

#include "protocols/MeterSOS_S0.hpp"
#include <VZException.hpp>

#include <sstream>
#include <string>
#include <vector>

namespace MeterSOS_S0_Helper {

bool hasEnding(const std::string &fullString, const std::string &ending) {
	if (fullString.length() >= ending.length()) {
		return (0 ==
				fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}

	return false;
}

void tokenize(const std::string &text, const std::string &delimiter,
			  std::vector<std::string> &data) {
	size_t start = text.find_first_not_of(delimiter);
	size_t end = start;

	while (start != std::string::npos) {
		end = text.find(delimiter, start);
		data.push_back(text.substr(start, end - start));
		start = text.find_first_not_of(delimiter, end);
	}
}

}; // namespace MeterSOS_S0_Helper

MeterSOS_S0::MeterSOS_S0(std::list<Option> &options)
	: Protocol("sos_s0"), _device(""), _read_timeout_s(10), _send_zero(false) {
	OptionList option_list;

	// connection
	try {
		_device = option_list.lookup_string(options, "device");
		if (!_device.length()) {
			throw vz::VZException("device without length");
		}
	} catch (vz::VZException &e) {
		print(log_alert, "Missing device or host", name().c_str());
		throw;
	}

	// baudrate and parity are hard coded for this devices
	_baudrate = B9600;
	_parity = parity_7n1;

	try {
		_read_timeout_s = option_list.lookup_int(options, "read_timeout");
	} catch (vz::OptionNotFoundException &e) {
		// use default: 10s from constructor
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse read_timeout", name().c_str());
		throw;
	}
	try {
		_send_zero = option_list.lookup_bool(options, "send_zero");
	} catch (vz::OptionNotFoundException &e) {
		// keep default init value (false)
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse send_zero", "");
		throw;
	}
}

MeterSOS_S0::~MeterSOS_S0() {}

int MeterSOS_S0::open() {

	if (_device.length() > 0) {
		_fd = _openDevice(&_oldtio, _baudrate);
	}

	return (_fd < 0) ? ERR : SUCCESS;
}

int MeterSOS_S0::close() { return ::close(_fd); }

ssize_t MeterSOS_S0::read(std::vector<Reading> &rds, size_t max_readings) {

	time_t start_time(0);
	time_t end_time(0);
	size_t total_bytes_read(0);
	unsigned char last_byte(0);
	bool end_line_found(false);

	// check file descriptor
	if (!_is_valid_fd()) {
		print(log_alert, " file descriptor [%i] to [%s] is invalid. try to reopen.", name().c_str(),
			  _fd, device());
		_fd = _openDevice(&_oldtio, _baudrate);
		if (_fd < 0) {
			return 0;
		}
	}

	time(&start_time);
	std::string strReadText;

	while (1) {
		_safe_to_cancel();
		// check for timeout
		time(&end_time);
		if (difftime(end_time, start_time) > _read_timeout_s) {
			print(log_error, " nothing received for more than %d seconds", name().c_str(),
				  _read_timeout_s);
			break;
		}

		// now read a single byte
		ssize_t bytes_read = ::read(_fd, &last_byte, 1);
		if (bytes_read == 0 || (bytes_read == -1 && errno == EAGAIN)) {
			// wait 5ms and read again
			usleep(5000);
			continue;
		} else if (bytes_read == -1) {
			print(log_error, " error reading a byte (%d)", name().c_str(), errno);
			break;
		}
		strReadText.push_back(last_byte);
		total_bytes_read++;

		if (MeterSOS_S0_Helper::hasEnding(strReadText, "\n\n")) {
			end_line_found = true;
			break;
		}

	} // end while

	if (end_line_found) {
		std::vector<std::string> data;
		MeterSOS_S0_Helper::tokenize(strReadText, ":", data);
		if (data.size() == 19) {

			char *end = nullptr;
			size_t deviceId(strtoul(data.at(1).c_str(), &end, 10));

			int tuplesFound(0);
			int dataStart(4);
			for (int count = 0; count < 5; count++) {
				std::string strIdent(data.at(dataStart));
				size_t value(strtoul(data.at(dataStart + 1).c_str(), &end, 10));

				if ((value > 0) || _send_zero) {
					tuplesFound++;
					rds[count].identifier(new StringIdentifier(strIdent));
					rds[count].time();
					rds[count].value(value);
					dataStart += 3;

					std::stringstream ss;
					ss << "deviceId: [" << deviceId << "] - ID: " << strIdent
					   << " value: " << value;
					print(log_debug, "read: %s", name().c_str(), ss.str().c_str());
				}
			}
			return tuplesFound; // return number of good readings so far.
		} else {
			print(log_alert,
				  "tokenize of returned text failed: %lu instead of 19 ELements in text found",
				  name().c_str(), data.size());
		}
	}

	return 0;
}

bool MeterSOS_S0::_is_valid_fd() {
	auto ret_code = fcntl(_fd, F_GETFL);
	return ((ret_code != -1) || (errno != EBADF)) ? true : false;
}

int MeterSOS_S0::_openDevice(struct termios *old_tio, speed_t baudrate) {
	struct termios tio;
	memset(&tio, 0, sizeof(struct termios));

	int fd = ::open(device(), O_RDWR | O_NOCTTY | O_NONBLOCK);
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

// snip from kernel end
