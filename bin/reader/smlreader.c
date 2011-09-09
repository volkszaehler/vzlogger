/**
 * SML protocol parsing
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @copyright Copyright (c) 2011, Juri Glass, Mathias Runge, Nadim El Sayed, DAI-Labor, TU-Berlin
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

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include <math.h>

#include <netdb.h>
#include <sys/socket.h>

#include <signal.h>

#include <sml/sml_file.h>
#include <sml/sml_transport.h>

#include "obis.h"
#include "unit.h"

obis_id_t filter;

void transport_receiver(unsigned char *buffer, size_t buffer_len) {
	/* strip escape sequences */
	sml_file *file = sml_file_parse(buffer + 8, buffer_len - 16);
	
	for (int i = 0; i < file->messages_len; i++) {
		sml_message *message = file->messages[i];
		
		if (*message->message_body->tag == SML_MESSAGE_GET_LIST_RESPONSE) {
			sml_list *entry;
			sml_get_list_response *body;
			
			body = (sml_get_list_response *) message->message_body->data;
			
			printf("new message from: %*s\n", body->server_id->len, body->server_id->str);
			
			for (entry = body->val_list; entry != NULL; entry = entry->next) { /* linked list */
				obis_id_t id = obis_init(entry->obj_name->str);
				
				if (memcmp(&id, &filter, sizeof(obis_id_t)) == 0) {
					struct timeval time;
					int unit = (entry->unit) ? *entry->unit : 0;
					int scaler = (entry->scaler) ? *entry->scaler : 1;
					double value;
				
					switch (entry->value->type) {
						case 0x51: value = *entry->value->data.int8; break;
						case 0x52: value = *entry->value->data.int16; break;
						case 0x54: value = *entry->value->data.int32; break;
						case 0x58: value = *entry->value->data.int64; break;
						case 0x61: value = *entry->value->data.uint8; break;
						case 0x62: value = *entry->value->data.uint16; break;
						case 0x64: value = *entry->value->data.uint32; break;
						case 0x68: value = *entry->value->data.uint64; break;
				
						default:
							fprintf(stderr, "Unknown value type: %x", type);
							value = 0;
					}
					
					/* apply scaler */
					value *= pow(10, scaler);
					
					
					/* get time */
					if (entry->val_time) { // TODO handle SML_TIME_SEC_INDEX
						time.tv_sec = *entry->val_time->data.timestamp;
						time.tv_usec = 0;
					}
					else {
						gettimeofday(&time, NULL);
					}

					printf("%lu.%lu\t%.2f %s\n", time.tv_sec, time.tv_usec, value, dlms_get_unit(unit));
				}
			}
		}
	}
	
	// free the malloc'd memory
	sml_file_free(file);
}

int open_socket(char *host, char *port) {
	struct sockaddr_in sin;
	struct addrinfo *ais;
	int fd;

	getaddrinfo(host, port, NULL, &ais);
	memcpy(&sin, ais->ai_addr, ais->ai_addrlen);
	
	printf("Opening socket\n");
	fd = socket(PF_INET, SOCK_STREAM, 0);
	
	printf("Conntecting to %s:%s\n", host, port);
	if (connect(fd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		perror(host);
		exit(EXIT_FAILURE);
	}
	
	return fd;
}

int main(int argc, char **argv) {
	char *host, *port, *obis;
	char buffer[16];
	int fd;
	
	host = (argc >= 2) ? argv[1] : "localhost";
	port = (argc >= 3) ? argv[2] : "7331";
	obis = (argc >= 4) ? argv[3] : "1-0:1.7.0"; /* total power */
	
	filter = obis_parse(obis);
	
	obis_unparse(filter, buffer);
	printf("Using OBIS Id: %s\n", buffer);
	
	fd = open_socket(host, port);
	
	if (fd > 0) {
		sml_transport_listen(fd, &transport_receiver);
		close(fd);
	}
	
	return 0;
}
