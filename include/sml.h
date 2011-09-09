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

#include "reading.h"
#include "obis.h"

typedef struct {
	int fd;
	obis_id_t id;	/* which OBIS we want to log */
	float counter;	/* Zählerstand */
	// termios etc..
} meter_handle_sml_t;

int meter_sml_open(meter_handle_sml_t *handle, char *options);
void meter_sml_close(meter_handle_sml_t *handle);
meter_reading_t meter_sml_read(meter_handle_sml_t *handle);

meter_reading_t meter_sml_parse(sml_file *file, obis_id_t which);

int meter_sml_open_port(char *device);
int meter_sml_open_socket(char *node, char *service);

#endif /* _SML_H_ */
