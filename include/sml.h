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
 
#ifndef _SML_H_
#define _SML_H_

#define SML_BUFFER_LEN 8096

#include <sml/sml_file.h>
#include <sml/sml_value.h>

#include "obis.h"

typedef struct {
	char *host;
	char *device;
	int baudrate;

	int fd;
	//termios old_tio;
} meter_handle_sml_t;

/* forward declarations */
struct meter;
struct reading;

/**
 * Cast arbitrary sized sml_value to double
 *
 * @param value the sml_value which should be casted
 * @return double value representation of sml_value, NAN if an error occured
 */
double sml_value_todouble(sml_value *value);

/**
 * Initialize meter structure with a list of options
 *
 * @param mtr the meter structure
 * @param options a list of options
 * @return 0 on success, <0 on error
 */
int meter_init_sml(struct meter *mtr, list_t options);

/**
 * Open connection via serial port or socket to meter
 *
 * @param mtr the meter structure
 * @return 0 on success, <0 on error
 */
int meter_open_sml(struct meter *mtr);

/**
 * Reset port/socket and freeing handle
 *
 * @param mtr the meter structure
 */
int meter_close_sml(struct meter *mtr);

/**
 * Blocking read on meter
 *
 * Most EDL conform meters periodically send data every
 * 3-4 seconds.
 *
 * @param mtr the meter structure
 * @param rds pointer to array of readings with size n
 * @param n size of the rds array
 * @return number of readings stored to rds
 */
size_t meter_read_sml(struct meter *mtr, struct reading *rds, size_t n);

/**
 * Parses SML list entry and stores it in reading pointed by rd
 *
 * @param list the list entry
 * @param rd the reading to store to
 */
void meter_sml_parse(sml_list *list, struct reading *rd);

/**
 * Open serial port by device
 *
 * @param device the device path, usually /dev/ttyS*
 * @return file descriptor, <0 on error
 */
int meter_sml_open_port(const char *device);

/**
 * Open socket
 *
 * @param node the hostname or ASCII encoded IP address
 * @param the ASCII encoded portnum or service as in /etc/services
 * @return file descriptor, <0 on error
 */
int meter_sml_open_socket(const char *node, const char *service);

#endif /* _SML_H_ */
