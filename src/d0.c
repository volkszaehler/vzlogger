/**
 * Plaintext protocol according to DIN EN 62056-21
 *
 * This protocol uses OBIS to identify the readout data
 *
 * This is our example protocol. Use this skeleton to add your own
 * protocols and meters.
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
 * @author Mathias Dalheimer <md@gonium.net>
 * based heavily on libehz (https://github.com/gonium/libehz.git)
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

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <regex.h>

#include "meter.h"
#include "obis.h"
#include "d0.h"

int meter_open_d0(meter_t *mtr) {
	meter_handle_d0_t *handle = &mtr->handle.d0;
	struct termios tio;

	memset(&tio, 0, sizeof(tio));

	/* open serial port */
	handle->fd = open(mtr->connection, O_RDWR);

	if (handle->fd < 0) {
        	return -1;
        }

	tio.c_iflag = 0;
	tio.c_oflag = 0;
	tio.c_cflag = B9600 | CS7 | CREAD | CLOCAL;
	tio.c_lflag = 0;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 20;

	return 0;
}

void meter_close_d0(meter_t *mtr) {
	meter_handle_d0_t *handle = &mtr->handle.d0;

	close(handle->fd);
}

size_t meter_read_d0(meter_t *mtr, reading_t rds[], size_t n) {
	meter_handle_d0_t *handle = &mtr->handle.d0;

	struct timeval time;
	enum { IDENTIFICATION, OBIS, VALUE, UNIT } context;

	char identification[20];	/* 3 vendor + 1 baudrate + 16 meter specific */
	char line[78];			/* 16 obis + '(' + 32 value + '*' + 16 unit + ')' + '\r' + '\n' */
	char byte;

	int fd = handle->fd;
	int i, j, m;

	/* wait for identification */
	while (read(fd, &byte, 1)) {
		if (byte == '/') { /* start of identification */
			for (i = 0; i < 16 && byte != '\r'; i++) {
				read(fd, &byte, 1);
				identification[i] = byte;
			}
			identification[i] = '\0';
			break;
		}
	}

	/* debug */
	printf("got message from %s\n", identification);

	/* take timestamp */
	gettimeofday(&time, NULL);

	/* read data lines: "obis(value*unit)" */
	m = 0;
	while (m < n && byte != '!') {

		meter_d0_parse_line(&rds[m], line, j);
	}

	return 1;
}

int meter_d0_parse_line(reading_t &rd, char *line, size_t n) {
	char id[16];
	char value[32];
	char unit[16];

	rds[m].time = time; /* use timestamp of data block arrival for all readings */
	rds[m].value = ;
	rds[m].identifier.obis = ;
}

