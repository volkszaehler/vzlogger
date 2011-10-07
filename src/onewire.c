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

#include "meter.h"
#include "onewire.h"

/**
 * Initialize sensor
 *
 * @param address path to the sensor in the owfs
 * @return pointer to file descriptor
 */
int meter_open_onewire(meter_t *mtr) {
	meter_handle_onewire_t *handle = &mtr->handle.onewire;

	handle->file = fopen(mtr->connection, "r");

	return (handle->file == NULL) ? -1 : 0;
}

void meter_close_onewire(meter_t *mtr) {
	meter_handle_onewire_t *handle = &mtr->handle.onewire;

	fclose(handle->file);
}

size_t meter_read_onewire(meter_t *mtr, reading_t rds[], size_t n) {
	meter_handle_onewire_t *handle = &mtr->handle.onewire;

	char buffer[16];
	int bytes;

	do {
		rewind(handle->file);
		bytes = fread(buffer, 1, 16, handle->file);
		buffer[bytes] = '\0'; /* zero terminated, required? */

		if (bytes) {
			rds->value = strtof(buffer, NULL);
			gettimeofday(&rds->time, NULL);
		}
	} while (rds->value == 85); /* skip invalid readings */

	return 1;
}
