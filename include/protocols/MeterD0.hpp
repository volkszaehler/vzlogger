/**
 * Plaintext protocol according to DIN EN 62056-21
 *
 * This protocol uses OBIS to identify the readout data
 * And is also sometimes called "D0"
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
 
#ifndef _D0_H_
#define _D0_H_

#define D0_BUFFER_LENGTH 1024

#include <termios.h>

#include <protocols/Protocol.hpp>

class MeterD0 : public vz::protocol::Protocol {
public:
	MeterD0(std::list<Option> options);
	virtual ~MeterD0();

	int open();
	int close();
	ssize_t read(std::vector<Reading> &rds, size_t n);

	const char *host() const { return _host.c_str(); }
	const char *device() const { return _device.c_str(); }

private:
	std::string _host;
	std::string _device;
	int _baudrate;

	int _fd; /* file descriptor of port */
	struct termios _oldtio; /* required to reset port */

	/**
	 * Open socket
	 *
	 * @param node the hostname or ASCII encoded IP address
	 * @param the ASCII encoded portnum or service as in /etc/services
	 * @return file descriptor, <0 on error
	 */
	int _openSocket(const char *node, const char *service);
	int _openDevice(struct termios *old_tio, speed_t baudrate);
};

int meter_d0_open_device(const char *device, struct termios *old_tio, speed_t baudrate);

#endif /* _D0_H_ */
