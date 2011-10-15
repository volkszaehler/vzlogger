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
#include <errno.h>

#include "meter.h"
#include "s0.h"

/**
 * Setup serial port
 */
int meter_open_s0(meter_t *mtr) {
	meter_handle_s0_t *handle = &mtr->handle.s0;

	/* open port */
	int fd = open(mtr->connection, O_RDWR | O_NOCTTY); 

        if (fd < 0) {
		perror(mtr->connection);
        	return -1;
        }

	/* save current port settings */
	tcgetattr(fd, &handle->oldtio);

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
	handle->fd = fd;

        return 0;
}

void meter_close_s0(meter_t *mtr) {
	meter_handle_s0_t *handle = &mtr->handle.s0;

	/* reset serial port */
	tcsetattr(handle->fd, TCSANOW, &handle->oldtio);

	/* close serial port */
	close(handle->fd);
}

size_t meter_read_s0(meter_t *mtr, reading_t rds[], size_t n) {
	meter_handle_s0_t *handle = &mtr->handle.s0;
	char buf[8];

	/* clear input buffer */
        tcflush(handle->fd, TCIOFLUSH);

	/* blocking until one character/pulse is read */
	read(handle->fd, buf, 8);

	/* store current timestamp */
	gettimeofday(&rds->time, NULL);
	rds->value = 1;

	/* wait some ms for debouncing */
	usleep(30000);

	return 1;
}

