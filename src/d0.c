/**
 * Plaintext protocol according to DIN EN 62056-21
 *
 * This protocol uses OBIS codes to identify the readout data
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
	int fd; // file descriptor
	int res;

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

size_t meter_read_d0(meter_t *mtr, reading_t rds[], size_t max_readings) {
	meter_handle_d0_t *handle = &mtr->handle.d0;

	enum { START, VENDOR, BAUDRATE, IDENTIFICATION, START_LINE, OBIS_CODE, VALUE, UNIT, END_LINE, END } context;

	char vendor[3+1];		/* 3 upper case vendor + '\0' termination */
	char identification[16+1];	/* 16 meter specific + '\0' termination */
	char obis_code[16+1];		/* A-B:C.D.E*F
					   fields A, B, E, F are optional
					   fields C & D are mandatory
					   A: energy type; 1: energy
					   B: channel number; 0: no channel specified
					   C: data items; 0-89 in COSEM context: IEC 62056-62, Clause D.1; 96: General service entries
						1:  Totel Active power+
						21: L1 Active power+
						31: L1 Current
						32: L1 Voltage
						41: L2 Active power+
						51: L2 Current
						52: L2 Voltage
						61: L3 Active power+
						71: L3 Current
						72: L3 Voltage
						96.1.255: Metering point ID 256 (electricity related) 
						96.5.5: Meter started status flag
					   D: types
					   E: further processing or classification of quantities
					   F: storage of data
						see DIN-EN-62056-61 */
	char value[32+1];		/* value, i.e. the actual reading */
	char unit[16+1];		/* the unit of the value, e.g. kWh, V, ... */

	char baudrate;			/* 1 byte for */
	char byte;			/* we parse our input byte wise */
	int byte_iterator; 
	int number_of_tuples;

	byte_iterator =  number_of_tuples = baudrate = 0;

	context = START;				/* start with context START */

	while (read(handle->fd, &byte, 1)) {
		if (byte == '/') context = START; 	/* reset to START if "/" reoccurs */
		else if (byte == '!') context = END;	/* "!" is the identifier for the END */
		switch (context) {
			case START:			/* strip the initial "/" */
				byte_iterator = number_of_tuples = 0;	/* start */
				context = VENDOR;	/* set new context: START -> VENDOR */
				break;

			case VENDOR:			/* VENDOR has 3 Bytes */
				if (!isalpha(byte)) goto error; /* Vendor ID needs to be alpha */
				vendor[byte_iterator++] = byte;	/* read next byte */
				if (byte_iterator >= 3) {	/* stop after 3rd byte */
					vendor[byte_iterator] = '\0'; /* termination */
					byte_iterator = 0;	/* reset byte counter */

					context = BAUDRATE;	/* set new context: VENDOR -> BAUDRATE */
				} 
				break;

			case BAUDRATE:			/* BAUDRATE consists of 1 char only */
				baudrate = byte;	
				context = IDENTIFICATION;	/* set new context: BAUDRATE -> IDENTIFICATION */
				byte_iterator = 0;
				break;

			case IDENTIFICATION:		/* IDENTIFICATION has 16 bytes */
				if (byte == '\r' || byte == '\n') { /* detect line end */
					identification[byte_iterator] = '\0'; /* termination */
					context = OBIS_CODE;	/* set new context: IDENTIFICATION -> OBIS_CODE */
					byte_iterator = 0;
				}
				else identification[byte_iterator++] = byte;
				break;

			case START_LINE:
				break;
			case OBIS_CODE:
				if ((byte != '\n') && (byte != '\r')) 
				{
					if (byte == '(') {
						obis_code[byte_iterator] = '\0';
						byte_iterator = 0;

						context = VALUE;
					}
					else obis_code[byte_iterator++] = byte;
				}
				break;

			case VALUE:
				if (byte == '*' || byte == ')') {
					value[byte_iterator] = '\0';
					byte_iterator = 0;

					if (byte == ')') {
						unit[0] = '\0';
						context =  END_LINE;
					}
					else {
						context = UNIT;
					}
				}
				else value[byte_iterator++] = byte;
				break;

			case UNIT:
				if (byte == ')') {
					unit[byte_iterator] = '\0';
					byte_iterator = 0;

					context = END_LINE;
				}
				else unit[byte_iterator++] = byte;
				break;

			case END_LINE:
				if (byte == '\r' || byte == '\n') {
					if ((number_of_tuples < max_readings) && (strlen(obis_code) > 0) && (strlen(value) > 0)) { /* free slots available and sain content? */
						printf("parsed reading (OBIS code=%s, value=%s, unit=%s)\n", obis_code, value, unit);
						rds[number_of_tuples].value = strtof(value, NULL);
						obis_parse(obis_code, &rds[number_of_tuples].identifier.obis);
						gettimeofday(&rds[number_of_tuples].time, NULL);

						byte_iterator = 0;
						number_of_tuples++;

						context = OBIS_CODE;
					}
				}
				break;

			case END:
				print(log_debug, "Read package with %i tuples (vendor=%s, baudrate=%c, identification=%s)", mtr, number_of_tuples, vendor, baudrate, identification);
				return number_of_tuples;
		}
	}

error:
	print(log_error, "Something unexpected happened: %s:%i!", mtr, __FUNCTION__, __LINE__);
	return 0;
}

