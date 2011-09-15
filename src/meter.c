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

#include "../include/meter.h"

/* List of available meter types */
const meter_type_t meter_types[] = {
	{ONEWIRE,	"onewire",	"Dallas 1-Wire sensors (via OWFS)",		1},
	{RANDOM,	"random",	"Random walk",					1},
	{S0,		"S0",		"S0 on RS232",					0},
//	{D0,		"D0",		"On-site plaintext protocol (DIN EN 62056-21)",	0},
#ifdef SML_SUPPORT
	{SML,		"sml",		"Smart Meter Language",				0},
#endif /* SML_SUPPORT */
	{} /* stop condition for iterator */
};
	
void meter_init(meter_t *meter, meter_type_t *type, char *options) {
	meter->type = type;
	meter->options = strdup(options);
}

void meter_free(meter_t *meter) {
	free(meter->options);
}

int meter_open(meter_t *meter) {
	switch (meter->type->tag) {
		case RANDOM:
			return meter_random_open(&meter->handle.random, meter->options);

		case S0:
			return meter_s0_open(&meter->handle.s0, meter->options);

		case D0:
			return meter_d0_open(&meter->handle.d0, meter->options);

#ifdef SML_SUPPORT
		case SML:
			return meter_sml_open(&meter->handle.sml, meter->options);
#endif /* SML_SUPPORT */

		case ONEWIRE:
			return meter_onewire_open(&meter->handle.onewire, meter->options);
			
		default:
			return -1;
	}
}

void meter_close(meter_t *meter) {
	switch (meter->type->tag) {
		case RANDOM:
			meter_random_close(&meter->handle.random);
			break;

		case S0:
			meter_s0_close(&meter->handle.s0);
			break;

		case D0:
			meter_d0_close(&meter->handle.d0);
			break;

#ifdef SML_SUPPORT
		case SML:
			meter_sml_close(&meter->handle.sml);
			break;
#endif /* SML_SUPPORT */

		case ONEWIRE:
			meter_onewire_close(&meter->handle.onewire);
			break;
	}
}

meter_reading_t meter_read(meter_t *meter) {
	meter_reading_t rd;
	
	switch (meter->type->tag) {
		case RANDOM:
			return meter_random_read(&meter->handle.random);

		case S0:
			return meter_s0_read(&meter->handle.s0);

		case D0:
			return meter_d0_read(&meter->handle.d0);

#ifdef SML_SUPPORT
		case SML:
			return meter_sml_read(&meter->handle.sml);
#endif /* SML_SUPPORT */

		case ONEWIRE:
			return meter_onewire_read(&meter->handle.onewire);
			
		default:
			return rd;
	}
}
