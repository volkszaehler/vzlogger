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
 *        - a,c,e,g,i = number of pulses since last message
 *        - b,d,f,h,j = total number of pulses since start of device
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Hans-Joerg Leu <info@steffenvogel.de>
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
// RW: added ack
#ifndef _SOS_S0_H_
#define _SOS_S0_H_

#include <termios.h>
#include <protocols/Protocol.hpp>

class MeterSOS_S0 : public vz::protocol::Protocol {
  public:
	MeterSOS_S0(std::list<Option> &options);
	virtual ~MeterSOS_S0();

	int open();
	int close();
	ssize_t read(std::vector<Reading> &rds, size_t n);
	virtual bool allowInterval() const {
		return false;
	} // only allow conf setting interval if pull is set (otherwise meter sends autom.)

	const char *device() const { return _device.c_str(); }

  private:
	std::string _device;
	std::string _dump_file;
	int _baudrate;
	parity_type_t _parity;
    size_t _read_timeout_s;

	int _fd; /* file descriptor of port */
	struct termios _oldtio; /* required to reset port */

	int _openDevice(struct termios *old_tio, speed_t baudrate);
    bool _is_valid_fd();
};

#endif /* _D0_H_ */
