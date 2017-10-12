/**
 * Read data from files & fifos
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
 
#ifndef _FILE_H_
#define _FILE_H_

#include <protocols/Protocol.hpp>

class MeterFile : public vz::protocol::Protocol {

public:
	MeterFile(std::list<Option> options);
	virtual ~MeterFile();

	int open();
	int close();
	ssize_t read(std::vector<Reading> &rds, size_t n);

	const char *path() { return _path.c_str(); }
	const char *format() { return _format.c_str(); }
  
  private:
	std::string _path;
	std::string _format;
	int _rewind;
	int _interval;

	FILE *_fd;
	int _notify_fd;
};

#endif /* _FILE_H_ */
