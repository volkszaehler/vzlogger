/**
 * Protocol interface
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../bin/logger/include/list.h"

#include "meter.h"

/* List of available meter types */
const meter_type_t meter_types[] = {
	{ONEWIRE,	"onewire",	"Dallas 1-Wire sensors (via OWFS)",		1, 1},
	{RANDOM,	"random",	"Random walk",					1, 1},
	{S0,		"s0",		"S0 on RS232",					1, 0},
//	{D0,		"d0",		"On-site plaintext protocol (DIN EN 62056-21)",	16, 0},
#ifdef SML_SUPPORT
	{SML,		"sml",		"Smart Meter Language",				16, 0},
#endif /* SML_SUPPORT */
	{} /* stop condition for iterator */
};

double tvtod(struct timeval tv) {
	return tv.tv_sec + tv.tv_usec / 1e6;
}

void meter_init(meter_t *mtr, const meter_type_t *type, const char *connection) {
	static int instances; /* static to generate channel ids */
	snprintf(mtr->id, 5, "mtr%i", instances++);

	mtr->type = type;
	mtr->connection = strdup(connection);
}

void meter_free(meter_t *mtr) {
	free(mtr->connection);
}

int meter_open(meter_t *mtr) {
	switch (mtr->type->id) {
		case RANDOM:  return meter_open_random(mtr);
		case S0:      return meter_open_s0(mtr);
		case D0:      return meter_open_d0(mtr);
		case ONEWIRE: return meter_open_onewire(mtr);
#ifdef SML_SUPPORT
		case SML:     return meter_open_sml(mtr);
#endif /* SML_SUPPORT */
		default: fprintf(stderr, "error: unknown meter type: %i\n", mtr->type->id);
	}
	
	return -1;
}

void meter_close(meter_t *mtr) {
	switch (mtr->type->id) {
		case RANDOM:  meter_close_random(mtr);  break;
		case S0:      meter_close_s0(mtr);      break;
		case D0:      meter_close_d0(mtr);      break;
		case ONEWIRE: meter_close_onewire(mtr); break;
#ifdef SML_SUPPORT
		case SML:     meter_close_sml(mtr);     break;
#endif /* SML_SUPPORT */
		default: fprintf(stderr, "error: unknown meter type: %i\n", mtr->type->id);
	}
}

size_t meter_read(meter_t *mtr, reading_t rds[], size_t n) {
	switch (mtr->type->id) {
		case RANDOM:  return meter_read_random(mtr, rds, n);
		case S0:      return meter_read_s0(mtr, rds, n);
		case D0:      return meter_read_d0(mtr, rds, n);
		case ONEWIRE: return meter_read_onewire(mtr, rds, n);
#ifdef SML_SUPPORT
		case SML:     return meter_read_sml(mtr, rds, n);
#endif /* SML_SUPPORT */
		default: fprintf(stderr, "error: unknown meter type: %i\n", mtr->type->id);
	}
	
	return 0;
}
