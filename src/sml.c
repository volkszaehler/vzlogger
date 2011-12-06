/**
 * Wrapper around libsml
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @copyright Copyright (c) 2011, DAI-Labor, TU-Berlin
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
 * @author Juri Glass
 * @author Mathias Runge
 * @author Nadim El Sayed
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
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

/* serial port */
#include <fcntl.h>
#include <sys/ioctl.h>

/* socket */ 
#include <netdb.h>
#include <sys/socket.h>

/* sml stuff */
#include <sml/sml_file.h>
#include <sml/sml_transport.h>

#include "meter.h"
#include "sml.h"
#include "obis.h"
#include "options.h"

int meter_init_sml(meter_t *mtr, list_t options) {
	meter_handle_sml_t *handle = &mtr->handle.sml;

	/* connection */
	char *host, *device;
	if (options_lookup_string(options, "host", &host) == SUCCESS) {
		handle->host = strdup(host);
		handle->device = NULL;
	}
	else if (options_lookup_string(options, "device", &device) == SUCCESS) {
		handle->device = strdup(device);
		handle->host = NULL;
	}
	else {
		print(log_error, "Missing device or host", mtr);
		return ERR;
	}

	/* baudrate */
	int baudrate;
	switch (options_lookup_int(options, "baudrate", &baudrate)) {
		case SUCCESS:
			/* find constant for termios structure */
			switch (baudrate) {
				case 1200: handle->baudrate = B1200; break;
				case 1800: handle->baudrate = B1800; break;
				case 2400: handle->baudrate = B2400; break;
				case 4800: handle->baudrate = B4800; break;
				case 9600: handle->baudrate = B9600; break;
				case 19200: handle->baudrate = B19200; break;
				case 38400: handle->baudrate = B38400; break;
				case 57600: handle->baudrate = B57600; break;
				case 115200: handle->baudrate = B115200; break;
				case 230400: handle->baudrate = B230400; break;
				default:
					print(log_error, "Invalid baudrate: %i", mtr, baudrate);
					return ERR;
			}
			break;

		case ERR_NOT_FOUND: /* using default value if not specified */
			handle->baudrate = B9600;
			break;

		default:
			print(log_error, "Failed to parse the baudrate", mtr);
			return ERR;
	}

	return SUCCESS;
}

int meter_open_sml(meter_t *mtr) {
	meter_handle_sml_t *handle = &mtr->handle.sml;

	if (handle->device != NULL) { /* local connection */
		handle->fd = meter_sml_open_device(handle->device, &handle->old_tio, handle->baudrate);
	}
	else if (handle->host != NULL) { /* remote connection */
		char *addr = strdup(handle->host);
		char *node = strsep(&addr, ":"); /* split port/service from hostname */
		char *service = strsep(&addr, ":");

		handle->fd = meter_sml_open_socket(node, service);

		free(addr);
	}

	return (handle->fd < 0) ? ERR : SUCCESS;
}

int meter_close_sml(meter_t *meter) {
	meter_handle_sml_t *handle = &meter->handle.sml;

	if (handle->device != NULL) {
		/* reset serial port */
		tcsetattr(handle->fd, TCSANOW, &handle->old_tio);
	}

	return close(handle->fd);
}

size_t meter_read_sml(meter_t *meter, reading_t rds[], size_t n) {
	meter_handle_sml_t *handle = &meter->handle.sml;

 	unsigned char buffer[SML_BUFFER_LEN];
	size_t bytes, m = 0;

	sml_file *file;
	sml_get_list_response *body;
	sml_list *entry;

	/* wait until a we receive a new datagram from the meter (blocking read) */
	bytes = sml_transport_read(handle->fd, buffer, SML_BUFFER_LEN);

	/* parse SML file & stripping escape sequences */
	file = sml_file_parse(buffer + 8, bytes - 16);

	/* obtain SML messagebody of type getResponseList */
	for (short i = 0; i < file->messages_len; i++) {
		sml_message *message = file->messages[i];

		if (*message->message_body->tag == SML_MESSAGE_GET_LIST_RESPONSE) {
			body = (sml_get_list_response *) message->message_body->data;
			entry = body->val_list;

			/* iterating through linked list */
			for (m = 0; m < n && entry != NULL; m++) {
				meter_sml_parse(entry, &rds[m]);
				entry = entry->next;
			}
		}
	}

	/* free the malloc'd memory */
	sml_file_free(file);

	return m+1;
}

void meter_sml_parse(sml_list *entry, reading_t *rd) {
	//int unit = (entry->unit) ? *entry->unit : 0;
	int scaler = (entry->scaler) ? *entry->scaler : 1;

	rd->value = sml_value_to_double(entry->value) * pow(10, scaler);

	obis_init(&rd->identifier.obis, entry->obj_name->str);

	// TODO handle SML_TIME_SEC_INDEX or time by SML File/Message
	if (entry->val_time) { /* use time from meter */
		rd->time.tv_sec = *entry->val_time->data.timestamp;
		rd->time.tv_usec = 0;
	}
	else {
		gettimeofday(&rd->time, NULL); /* use local time */
	}
}

int meter_sml_open_socket(const char *node, const char *service) {
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

int meter_sml_open_device(const char *device, struct termios *old_tio, speed_t baudrate) {
	int bits;
	struct termios tio;
	memset(&tio, 0, sizeof(struct termios));

	int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0) {
		print(log_error, "open(%s): %s", NULL, device, strerror(errno));
		return ERR;
	}

	/* enable RTS as supply for infrared adapters */
	ioctl(fd, TIOCMGET, &bits);
	bits |= TIOCM_RTS;
	ioctl(fd, TIOCMSET, &bits);

	/* get old configuration */
	tcgetattr(fd, &tio) ;

	/* backup old configuration to restore it when closing the meter connection */
	memcpy(old_tio, &tio, sizeof(struct termios));

	/*  set 8-N-1 */
	tio.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tio.c_oflag &= ~OPOST;
	tio.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tio.c_cflag &= ~(CSIZE | PARENB | PARODD | CSTOPB);
	tio.c_cflag |= CS8;

	/* set baudrate */
	cfsetispeed(&tio, baudrate);
	cfsetospeed(&tio, baudrate);

	/* apply new configuration */
	tcsetattr(fd, TCSANOW, &tio);

	return fd;
}
