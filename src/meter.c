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
#include <stdio.h> 

#include "meter.h"
#include "options.h"

#define METER_DETAIL(NAME, DESC, MAX_RDS, PERIODIC) { meter_protocol_##NAME, #NAME, DESC, MAX_RDS, PERIODIC, meter_init_##NAME, meter_open_##NAME, meter_close_##NAME, meter_read_##NAME }

static const meter_details_t protocols[] = {
/*	     alias	description						max_rds	periodic
===============================================================================================*/
METER_DETAIL(file, 	"Read from file (ex. 1-Wire sensors via OWFS)",		32,	TRUE),
//METER_DETAIL(exec, 	"Read from program (ex. 1-Wire sensors via digitemp)",	32,	TRUE),
METER_DETAIL(random,	"Random walk",						1,	TRUE),
METER_DETAIL(fluksov2,	"Read from onboard SPI of a Flukso v2",			16,	FALSE),
METER_DETAIL(s0,	"S0 on RS232",						1,	TRUE),
METER_DETAIL(d0,	"Plaintext protocol (DIN EN 62056-21)",			32,	FALSE),
#ifdef SML_SUPPORT
METER_DETAIL(sml,	"Smart Meter Language",					32,	FALSE),
#endif /* SML_SUPPORT */
{} /* stop condition for iterator */
};

int meter_init(meter_t *mtr, list_t options) {
	static int instances; /* static to generate unique channel ids */
	snprintf(mtr->id, 5, "mtr%i", instances++); /* set/increment id */

	/* protocol */
	char *protocol_str;
	if (options_lookup_string(options, "protocol", &protocol_str) != SUCCESS) {
		print(log_error, "Missing protocol or invalid type", mtr);
		return ERR;
	}

	if (meter_lookup_protocol(protocol_str, &mtr->protocol) != SUCCESS) {
		print(log_error, "Invalid protocol: %s", mtr, protocol_str);
		return ERR; /* skipping this meter */
	}

	/* interval */
	mtr->interval = -1; /* indicates unknown interval */
	if (options_lookup_int(options, "interval", &mtr->interval) == ERR_INVALID_TYPE) {
		print(log_error, "Invalid type for interval", mtr);
		return ERR;
	}

	const meter_details_t *details = meter_get_details(mtr->protocol);
	if (details->periodic == TRUE && mtr->interval < 0) {
		print(log_error, "Interval has to be positive!", mtr);
	} 

	return details->init_func(mtr, options);
}

int meter_lookup_protocol(const char* name, meter_protocol_t *protocol) {
	for (const meter_details_t *it = meter_get_protocols(); it != NULL; it++) {
		if (strcmp(it->name, name) == 0) {
			*protocol = it->id;
			return SUCCESS;
		}
	}
	return ERR_NOT_FOUND;
}

int meter_open(meter_t *mtr) {
	const meter_details_t *details = meter_get_details(mtr->protocol);
	return details->open_func(mtr);
}

int meter_close(meter_t *mtr) {
	const meter_details_t *details = meter_get_details(mtr->protocol);
	return details->close_func(mtr);
}

size_t meter_read(meter_t *mtr, reading_t rds[], size_t n) {
	const meter_details_t *details = meter_get_details(mtr->protocol);
	return details->read_func(mtr, rds, n);
}

const meter_details_t * meter_get_protocols() {
	return protocols;
}

const meter_details_t * meter_get_details(meter_protocol_t protocol) {
	for (const meter_details_t *it = protocols; it != NULL; it++) {
		if (it->id == protocol) {
			return it;
		}
	}
	return NULL;
}

