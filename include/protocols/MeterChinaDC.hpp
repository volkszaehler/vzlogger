/**
 * crappy chinese hall-effect DC meter with serial output
 *
 * (code based on MeterD0)
 *
 * @package vzlogger
 * @copyright Copyright (c) 2021, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
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
#ifndef _CHINADC_H_
#define _CHINADC_H_

#include <termios.h>

#include <protocols/Protocol.hpp>

class MeterChinaDC : public vz::protocol::Protocol {
  public:
	MeterChinaDC(std::list<Option> &options);
	virtual ~MeterChinaDC();

	int open();
	int close();
	ssize_t read(std::vector<Reading> &rds, size_t n);
	virtual bool allowInterval() const { return false; }
	const char *device() const { return _device.c_str(); }

  private:
	std::string _device;

	int _read_timeout_s;

	int _fd;                /* file descriptor of port */
	struct termios _oldtio; /* required to reset port */

	int _openDevice(struct termios *old_tio);
};

#endif /* _CHINADC_H_ */
