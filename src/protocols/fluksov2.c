/**
 * Parsing SPI output of the new fluksometer
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "meter.h"
#include "protocols/fluksov2.h"
#include "options.h"

int meter_init_fluksov2(meter_t *mtr, list_t options) {
	meter_handle_fluksov2_t *handle = &mtr->handle.fluksov2;

	char *fifo;
	if (options_lookup_string(options, "fifo", &fifo) == SUCCESS) {
		handle->fifo = strdup(fifo);
	}
	else {
		handle->fifo = strdup(FLUKSOV2_DEFAULT_FIFO); /* use default path */
	}

	return SUCCESS;
}

void meter_free_fluksov2(meter_t *mtr) {
	meter_handle_fluksov2_t *handle = &mtr->handle.fluksov2;

	free(handle->fifo);
}

int meter_open_fluksov2(meter_t *mtr) {
	meter_handle_fluksov2_t *handle = &mtr->handle.fluksov2;

	/* open port */
	handle->fd = open(handle->fifo, O_RDONLY); 

        if (handle->fd < 0) {
		print(log_error, "open(%s): %s", mtr, handle->fifo, strerror(errno));
        	return ERR;
        }

        return SUCCESS;
}

int meter_close_fluksov2(meter_t *mtr) {
	meter_handle_fluksov2_t *handle = &mtr->handle.fluksov2;

	return close(handle->fd); /* close fifo */
}

size_t read_line(int fd, char  *buffer, size_t n) {
	int i = 0;	/* iterator for buffer */
	char c;		/* character buffer */

	while (i < n) {
		int r = read(fd, &c, 1); /* read byte-per-byte, to identify a line break */

		if (r == 1) { /* successful read */
			switch (c) {
				case '\n': /* line delimiter */
					return i;

				default: /* normal character */
					buffer[i++] = c;
			}
		}
		else if (r < 0) { /* an error occured, pass through to caller */
			return r;
		}
	}

	return i;
}

size_t meter_read_fluksov2(meter_t *mtr, reading_t rds[], size_t n) {
	meter_handle_fluksov2_t *handle = &mtr->handle.fluksov2;

	size_t i = 0;		/* number of readings */
	size_t bytes = 0;	/* read_line() return code */
	char line[64];		/* stores each line read */
	char *cursor = line;	/* moving cursor for strsep() */

	do {
		bytes = read_line(handle->fd, line, 64); /* blocking read of a complete line */
		if (bytes < 0) {
			print(log_error, "read_line(%s): %s", mtr, handle->fifo, strerror(errno));
			return bytes; /* an error occured, pass through to caller */
		}
	} while (bytes == 0);


	char *time_str = strsep(&cursor, " \t"); /* first token is the timestamp */
	struct timeval time = {
		.tv_sec = strtol(time_str, NULL, 10),
		.tv_usec = 0 /* no millisecond resolution available */
	};

	while (cursor) {
		int channel = atoi(strsep(&cursor, " \t")) + 1; /* increment by 1 to distinguish between +0 and -0 */

		/* consumption */
		rds[i].time = time;
		rds[i].identifier.channel = -channel; /* consumption gets negative channel id as identifier! */
		rds[i].value = atoi(strsep(&cursor, " \t"));
		i++;

		/* power */
		rds[i].time = time;
		rds[i].identifier.channel = channel; /* power gets positive channel id as identifier! */
		rds[i].value = atoi(strsep(&cursor, " \t"));
		i++;
	}

	return i;
}

