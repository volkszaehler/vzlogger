/**
 * Replacement for fluksod by directly parsing the fluksometers SPI output
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
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

#ifndef _FLUKSOV2_H_
#define _FLUKSOV2_H_

#include <protocols/Protocol.hpp>

class MeterFluksoV2 : public vz::protocol::Protocol {

  public:
	MeterFluksoV2(std::list<Option> options);
	virtual ~MeterFluksoV2();

	int open();
	int close();
	ssize_t read(std::vector<Reading> &rds, size_t n);

  private:
	ssize_t _read_line(int fd, char *buffer, size_t n);

  private:
	const char *_fifo;
	int _fd; /* file descriptor of fifo */

	// const char *DEFAULT_FIFO = "/var/run/spid/delta/out";
	// const char *_DEFAULT_FIFO;
};

#endif /* _FLUKSOV2_H_ */
