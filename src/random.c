/**
 * Generate pseudo random data series with a random walk
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
 * 
 * "connection" sets the maximum value
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
#include <time.h>
#include <sys/time.h>

#include "meter.h"
#include "random.h"
#include "options.h"

int meter_init_random(meter_t *mtr, list_t options) {
	meter_handle_random_t *handle = &mtr->handle.random;

	handle->min = 0;
	handle->max = 40;
	handle->last = (handle->max + handle->min) / 2; /* start in the middle */

	if (options_lookup_double(options, "min", &handle->min) == ERR_INVALID_TYPE) {
		print(log_error, "Min value has to be a floating point number (e.g. '40.0')", mtr);
		return ERR;
	}

	if (options_lookup_double(options, "max", &handle->max) == ERR_INVALID_TYPE) {
		print(log_error, "Max value has to be a floating point number (e.g. '40.0')", mtr);
		return ERR;
	}

	return SUCCESS;
}

int meter_open_random(meter_t *mtr) {
	//meter_handle_random_t *handle = &mtr->handle.random;

	// TODO rewrite to use /dev/u?random
	srand(time(NULL)); /* initialize PNRG */

	return SUCCESS; /* can't fail */
}

int meter_close_random(meter_t *mtr) {
	//meter_handle_random_t *handle = &mtr->handle.random;

	return SUCCESS;
}

size_t meter_read_random(meter_t *mtr, reading_t rds[], size_t n) {
	meter_handle_random_t *handle = &mtr->handle.random;

	double step = ltqnorm((float) rand() / RAND_MAX);
	double new = handle->last + step;

	/* check boundaries */
	handle->last += (new > handle->max || new < handle->min) ? -step : step;

	rds->value = handle->last;
	gettimeofday(&rds->time, NULL);

	return 1;
}
