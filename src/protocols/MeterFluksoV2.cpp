/**
 * Parsing SPI output of the new fluksometer
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

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "threads.h"

#include "Options.hpp"
#include "protocols/MeterFluksoV2.hpp"
#include <VZException.hpp>

#define FLUKSOV2_DEFAULT_FIFO "/var/run/spid/delta/out"

MeterFluksoV2::MeterFluksoV2(std::list<Option> options) : Protocol("fluksov2") {
	OptionList optlist;

	try {
		_fifo = optlist.lookup_string(options, "fifo");
	} catch (vz::OptionNotFoundException &e) {
		_fifo = strdup(FLUKSOV2_DEFAULT_FIFO); /* use default path */
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse fifo", name().c_str());
		throw;
	}
}

MeterFluksoV2::~MeterFluksoV2() {

	// free((void *)_fifo);
}

int MeterFluksoV2::open() {

	/* open port */
	_fd = ::open(_fifo, O_RDONLY);

	if (_fd < 0) {
		print(log_alert, "open(%s): %s", name().c_str(), _fifo, strerror(errno));
		return ERR;
	}

	return SUCCESS;
}

int MeterFluksoV2::close() { return ::close(_fd); /* close fifo */ }

ssize_t MeterFluksoV2::read(std::vector<Reading> &rds, size_t n) {

	size_t i = 0;        /* number of readings */
	ssize_t bytes = 0;   /* read_line() return code */
	char line[64];       /* stores each line read */
	char *cursor = line; /* moving cursor for strsep() */

	do {
		bytes = _read_line(_fd, line, 64); /* blocking read of a complete line */
		if (bytes < 0) {
			print(log_alert, "read_line(%s): %s", name().c_str(), _fifo, strerror(errno));
			return bytes; /* an error occured, pass through to caller */
		}
	} while (bytes == 0);

	char *time_str = strsep(&cursor, " \t"); /* first token is the timestamp */
	struct timeval time;
	time.tv_sec = strtol(time_str, NULL, 10);
	time.tv_usec = 0; /* no millisecond resolution available */

	while (cursor) {
		_safe_to_cancel();
		int channel =
			atoi(strsep(&cursor, " \t")) + 1; /* increment by 1 to distinguish between +0 and -0 */

		/* consumption - gets negative channel id as identifier! */
		ReadingIdentifier *rid1(new ChannelIdentifier(-channel));
		rds[i].time(time);
		rds[i].identifier(rid1);
		rds[i].value(atoi(strsep(&cursor, " \t")));
		i++;

		/* power - gets positive channel id as identifier! */
		ReadingIdentifier *rid2(new ChannelIdentifier(channel));
		rds[i].time(time);
		rds[i].identifier(rid2);
		rds[i].value(atoi(strsep(&cursor, " \t")));
		i++;
	}

	return i;
}

ssize_t MeterFluksoV2::_read_line(int fd, char *buffer, size_t n) {
	size_t i = 0; /* iterator for buffer */
	char c;       /* character buffer */

	while (i < n) {
		ssize_t r = ::read(fd, &c, 1); /* read byte-per-byte, to identify a line break */

		if (r == 1) { /* successful read */
			switch (c) {
			case '\n': /* line delimiter */
				return i;

			default: /* normal character */
				buffer[i++] = c;
			}
		} else if (r < 0) { /* an error occured, pass through to caller */
			return r;
		}
	}

	return i;
}
