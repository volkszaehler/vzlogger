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

/* serial port */
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

/* socket */ 
#include <netdb.h>
#include <sys/socket.h>

/* sml stuff */
#include <sml/sml_file.h>
#include <sml/sml_transport.h>

#include "meter.h"
#include "sml.h"
#include "obis.h"

int meter_open_sml(meter_t *mtr) {
	meter_handle_sml_t *handle = &mtr->handle.sml;

	char *addr = strdup(mtr->connection);
	char *node = strsep(&addr, ":");
	char *service = strsep(&addr, ":");

	handle->fd = meter_sml_open_socket(node, service);
	//handle->fd = meter_sml_open_port(args);

	free(addr);

	return (handle->fd < 0) ? -1 : 0;
}

void meter_close_sml(meter_t *meter) {
	meter_handle_sml_t *handle = &meter->handle.sml;

	// TODO reset serial port
	close(handle->fd);
}

size_t meter_read_sml(meter_t *meter, reading_t rds[], size_t n) {
	meter_handle_sml_t *handle = &meter->handle.sml;

 	unsigned char buffer[SML_BUFFER_LEN];
	size_t bytes, m = 0;

	sml_file *file;
	sml_get_list_response *body;
	sml_list *entry;

	/* blocking read from fd */
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
	rd->identifier.obis = obis_init(entry->obj_name->str);

	/* get time */
	// TODO handle SML_TIME_SEC_INDEX or time by SML File/Message
	if (entry->val_time) {
		rd->time.tv_sec = *entry->val_time->data.timestamp;
		rd->time.tv_usec = 0;
	}
	else {
		gettimeofday(&rd->time, NULL);
	}
}

int meter_sml_open_socket(const char *node, const char *service) {
	struct sockaddr_in sin;
	struct addrinfo *ais;
	int fd, res;

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		fprintf(stderr, "error: socket(): %s\n", strerror(errno));
		return -1;
	}

	getaddrinfo(node, service, NULL, &ais);
	memcpy(&sin, ais->ai_addr, ais->ai_addrlen);
	freeaddrinfo(ais);

	res = connect(fd, (struct sockaddr *) &sin, sizeof(sin));
	if (res < 0) {
		fprintf(stderr, "error: connect(%s, %s): %s\n", node, service, strerror(errno));
		return -1;
	}

	return fd;
}

int meter_sml_open_port(const char *device) {
	int bits;
	struct termios config;
	memset(&config, 0, sizeof(config));

	int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0) {
		fprintf(stderr, "error: open(%s): %s\n", device, strerror(errno));
		return -1;
	}

	// set RTS
	ioctl(fd, TIOCMGET, &bits);
	bits |= TIOCM_RTS;
	ioctl(fd, TIOCMSET, &bits);

	tcgetattr(fd, &config) ;

	// set 8-N-1
	config.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	config.c_oflag &= ~OPOST;
	config.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	config.c_cflag &= ~(CSIZE | PARENB | PARODD | CSTOPB);
	config.c_cflag |= CS8;

	// set speed to 9600 baud
	cfsetispeed( &config, B9600);
	cfsetospeed( &config, B9600);

	tcsetattr(fd, TCSANOW, &config);
	return fd;
}
