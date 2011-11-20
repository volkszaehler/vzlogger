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
#include <ctype.h>
#include <sys/time.h>

/* socket */ 
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>

#include "meter.h"
#include "d0.h"
#include "obis.h"
#include "options.h"

int meter_d0_open_socket(const char *node, const char *service) {
	struct sockaddr_in sin;
	struct addrinfo *ais;
	int fd, res;

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		print(log_error, "socket(): %s", NULL, strerror(errno));
		return ERR;
	}

	getaddrinfo(node, service, NULL, &ais);
	memcpy(&sin, ais->ai_addr, ais->ai_addrlen);
	freeaddrinfo(ais);

	res = connect(fd, (struct sockaddr *) &sin, sizeof(sin));
	if (res < 0) {
		print(log_error, "connect(%s, %s): %s", NULL, node, service, strerror(errno));
		return ERR;
	}

	return fd;
}

int meter_init_d0(meter_t *mtr, list_t options) {
	meter_handle_d0_t *handle = &mtr->handle.d0;

	/* connection */
	handle->host = NULL;
	handle->device = NULL;
	if (options_lookup_string(options, "host", &handle->host) != SUCCESS && options_lookup_string(options, "device", &handle->device) != SUCCESS) {
		print(log_error, "Missing host or port", mtr);
		return ERR;
	}

	/* baudrate */
	handle->baudrate = 9600;
	if (options_lookup_int(options, "baudrate", &handle->baudrate) == ERR_INVALID_TYPE) {
		print(log_error, "Invalid type for baudrate", mtr);
		return ERR;
	}

	return SUCCESS;
}

int meter_open_d0(meter_t *mtr) {
	meter_handle_d0_t *handle = &mtr->handle.d0;

	if (handle->device != NULL) {
		print(log_error, "TODO: implement serial interface", mtr);
		return ERR;
	}
	else if (handle->host != NULL) {
		char *addr = strdup(handle->host);
		char *node = strsep(&addr, ":");
		char *service = strsep(&addr, ":");

		handle->fd = meter_d0_open_socket(node, service);
	}

	return (handle->fd < 0) ? ERR : SUCCESS;
}

int meter_close_d0(meter_t *mtr) {
	meter_handle_d0_t *handle = &mtr->handle.d0;

	return close(handle->fd);
}

size_t meter_read_d0(meter_t *mtr, reading_t rds[], size_t n) {
	meter_handle_d0_t *handle = &mtr->handle.d0;

	enum { START, VENDOR, BAUD, IDENT, START_LINE, OBIS, VALUE, UNIT, END_LINE, END } context;

	char vendor[3+1];		/* 3 upper case vendor + '\0' termination */
	char identification[16+1];	/* 16 meter specific + '\0' termination */
	char id[16+1];
	char value[32+1];
	char unit[16+1];

	char baudrate;		/* 1 byte */
	char byte;
	int j, k, m;

	j = k = m = baudrate = 0;

	context = START;

	while (read(handle->fd, &byte, 1)) {
		if (byte == '/') context = START;
		else if (byte == '!') context = END;

		switch (context) {
			case START:
				if (byte == '/') {
					j = k = m = 0;
					context = VENDOR;
				}
				break;

			case VENDOR:
				if (!isalpha(byte)) goto error;
				else vendor[j++] = byte;

				if (j >= 3) {
					vendor[j] = '\0'; /* termination */
					j = k = 0;

					context = BAUD;
				}
				break;

			case BAUD:
				baudrate = byte;
				context = IDENT;
				j = k = 0;
				break;

			case IDENT:
				/* data block starts after twice a '\r\n' sequence */
				/* b= CR LF CR LF */
				/* k=  1  2  3  4 */
				if (byte == '\r' || byte == '\n') {
					k++;
					if  (k >= 4) {
						identification[j] = '\0'; /* termination */
						j = k = 0;

						context = START_LINE;
					}
				}
				else identification[j++] = byte;
				break;

			case START_LINE:
			case OBIS:
				if (byte == '(') {
					id[j] = '\0';
					j = k = 0;

					context = VALUE;
				}
				else id[j++] = byte;
				break;

			case VALUE:
				if (byte == '*' || byte == ')') {
					value[j] = '\0';
					j = k = 0;

					if (byte == ')') {
						unit[0] = '\0';
						context =  END_LINE;
					}
					else {
						context = UNIT;
					}
				}
				else value[j++] = byte;
				break;

			case UNIT:
				if (byte == ')') {
					unit[j] = '\0';
					j = k = 0;

					context = END_LINE;
				}
				else unit[j++] = byte;
				break;

			case END_LINE:
				if (byte == '\r' || byte == '\n') {
					k++;
					if  (k >= 2) {
						if (m < n) { /* free slots available? */
							//printf("parsed reading (id=%s, value=%s, unit=%s)\n", id, value, unit);
							rds[m].value = strtof(value, NULL);
							obis_parse(id, &rds[m].identifier.obis);
							gettimeofday(&rds[m].time, NULL);

							j = k = 0;
							m++;
						}

						context = START_LINE;
					}
				}
				break;

			case END:
				print(log_info, "Read package with %i tuples (vendor=%s, baudrate=%c, ident=%s)", mtr, m, vendor, baudrate, identification);
				return m;
		}
	}

error:
	print(log_error, "Something unexpected happened: %s:%i!", mtr, __FUNCTION__, __LINE__);
	return 0;
}

