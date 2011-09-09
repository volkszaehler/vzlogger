/**
 * Generate pseudo random data series with a random walk
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../include/random.h"

int meter_random_open(meter_handle_random_t *handle, char *options) {
	srand(time(NULL)); /* initialize PNRG */

	handle->min = 0; // TODO parse from options
	handle->max = strtof(options, NULL);
	handle->last = handle->max * ((float) rand() / RAND_MAX); /* start value */

	return 0; /* always succeeds */
}

void meter_random_close(meter_handle_random_t *handle) {
	/* nothing todo */
}

meter_reading_t meter_random_read(meter_handle_random_t *handle) {
	meter_reading_t rd;

	handle->last += ltqnorm((float) rand() / RAND_MAX);

	/* check bounaries */
	if (handle->last > handle->max) {
		handle->last = handle->max;
	}
	else if (handle->last < handle->min) {
		handle->last = handle->min;
	}

	rd.value = handle->last;
	gettimeofday(&rd.tv, NULL);

	return rd;
}
