/**
 * Plaintext protocol according to DIN EN 62056-21
 *
 * This protocol uses OBIS to identify the readout data
 * And is also sometimes called "D0"
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
 
#ifndef _D0_H_
#define _D0_H_

#define D0_BUFFER_LENGTH 1024

#include <termios.h>

typedef struct {
	char *host;
	char *device;
	int baudrate;

	int fd; /* file descriptor of port */
	struct termios oldtio; /* required to reset port */
} meter_handle_d0_t;

/* forward declarations */
struct meter;
struct reading;

int meter_init_d0(struct meter *mtr, list_t options);
int meter_open_d0(struct meter *mtr);
int meter_close_d0(struct meter *mtr);
size_t meter_read_d0(struct meter *mtr, struct reading *rds, size_t n);

/**
 * Open socket
 *
 * @param node the hostname or ASCII encoded IP address
 * @param the ASCII encoded portnum or service as in /etc/services
 * @return file descriptor, <0 on error
 */
int meter_d0_open_socket(const char *node, const char *service);

#endif /* _D0_H_ */
