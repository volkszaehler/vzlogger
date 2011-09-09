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

#include "../include/sml.h"
#include "../include/obis.h"

int meter_sml_open(meter_handle_sml_t *handle, char *options) {
	char *node = strsep(&options, ":");
	char *service = strsep(&options, ":");
	
	handle->id = obis_parse(options);
	handle->fd = meter_sml_open_socket(node, service);
	//handle->fd = meter_sml_open_port(options);
	
	return (handle->fd < 0) ? -1 : 0;
}

void meter_sml_close(meter_handle_sml_t *handle) {
	// TODO reset serial port
	close(handle->fd);
}

meter_reading_t meter_sml_read(meter_handle_sml_t *handle) {
 	unsigned char buffer[SML_BUFFER_LEN];
	size_t bytes;
	sml_file *sml_file;
	meter_reading_t rd;
	
	/* blocking read from fd */
	bytes = sml_transport_read(handle->fd, buffer, SML_BUFFER_LEN);
	
	/* sml parsing & stripping escape sequences */
	sml_file = sml_file_parse(buffer + 8, bytes - 16);
	
	/* extraction of readings */
	rd = meter_sml_parse(sml_file, handle->id);

	/* free the malloc'd memory */
	sml_file_free(sml_file);
	
	return rd;
}

int meter_sml_open_socket(char *node, char *service) {
	struct sockaddr_in sin;
	struct addrinfo *ais;
	int fd, res;
	
	getaddrinfo(node, service, NULL, &ais);
	memcpy(&sin, ais->ai_addr, ais->ai_addrlen);
	
	fd = socket(PF_INET, SOCK_STREAM, 0);

	res = connect(fd, (struct sockaddr *) &sin, sizeof(sin));
	
	return (res < 0) ? -1 : fd;
}

int meter_sml_open_port(char *device) {
	int bits;
	struct termios config;
	memset(&config, 0, sizeof(config));

	int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0) {
		printf("error: open(%s): %s\n", device, strerror(errno));
		return -1;
	}

	// set RTS
	ioctl(fd, TIOCMGET, &bits);
	bits |= TIOCM_RTS;
	ioctl(fd, TIOCMSET, &bits);

	tcgetattr( fd, &config ) ;

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

meter_reading_t meter_sml_parse(sml_file *file, obis_id_t which) {
	meter_reading_t rd;

	for (int i = 0; i < file->messages_len; i++) {
		sml_message *message = file->messages[i];
		
		if (*message->message_body->tag == SML_MESSAGE_GET_LIST_RESPONSE) {
			sml_list *entry;
			sml_get_list_response *body;
			
			body = (sml_get_list_response *) message->message_body->data;
			
			for (entry = body->val_list; entry != NULL; entry = entry->next) { /* linked list */
				obis_id_t id = obis_init(entry->obj_name->str);
				
				if (obis_compare(which, id) == 0) {
					//int unit = (entry->unit) ? *entry->unit : 0;
					int scaler = (entry->scaler) ? *entry->scaler : 1;
				
					switch (entry->value->type) {
						case 0x51: rd.value = *entry->value->data.int8; break;
						case 0x52: rd.value = *entry->value->data.int16; break;
						case 0x54: rd.value = *entry->value->data.int32; break;
						case 0x58: rd.value = *entry->value->data.int64; break;
						case 0x61: rd.value = *entry->value->data.uint8; break;
						case 0x62: rd.value = *entry->value->data.uint16; break;
						case 0x64: rd.value = *entry->value->data.uint32; break;
						case 0x68: rd.value = *entry->value->data.uint64; break;
				
						default:
							fprintf(stderr, "Unknown value type: %x", entry->value->type);
					}
					
					/* apply scaler */
					rd.value *= pow(10, scaler);
					
					
					/* get time */
					if (entry->val_time) { // TODO handle SML_TIME_SEC_INDEX
						rd.tv.tv_sec = *entry->val_time->data.timestamp;
						rd.tv.tv_usec = 0;
					}
					else {
						gettimeofday(&rd.tv, NULL);
					}
					
					return rd; /* skipping rest */
				}
			}
		}
	}
	
	return rd;
}
