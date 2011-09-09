/**
 * Wrapper to read Dallas 1-wire Sensors via the 1-wire Filesystem (owfs)
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

#include <stdlib.h>

#include "../include/onewire.h"

/**
 * Initialize sensor
 *
 * @param address path to the sensor in the owfs
 * @return pointer to file descriptor
 */
int meter_onewire_open(meter_handle_onewire_t *handle, char *options) {
	handle->file  = fopen(options, "r");

	return (handle->file == NULL) ? -1 : 0;
}

void meter_onewire_close(meter_handle_onewire_t *handle) {
	fclose(handle->file);
}

meter_reading_t meter_onewire_read(meter_handle_onewire_t *handle) {
	meter_reading_t rd;
	char buffer[16];
	int bytes;

	do {
		rewind(handle->file);
		bytes = fread(buffer, 1, 16, handle->file);
		buffer[bytes] = '\0'; /* zero terminated, required? */

		if (bytes) {
			rd.value = strtof(buffer, NULL);
			gettimeofday(&rd.tv, NULL);
		}
	} while (rd.value == 85); /* skip invalid readings */

	return rd;
}
