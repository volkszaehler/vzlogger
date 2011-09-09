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

#ifndef _METER_H_
#define _METER_H_

/**
 * We have 2 diffrent protocol types:
 * - SENSOR:	a readout is triggered in equidistant intervals by calling
 *		the read function with an POSIX timer.
 *		The interval is set in the configuration.
 * - METER:	the meter itselfs triggers a readout.
 *		The pointer to the read function shoul be NULL.
 *		The 'interval' column in the configuration as no meaning.
 */

#include "../config.h"
#include "reading.h"

/* meter types */
#include "random.h"
#include "s0.h"
#include "d0.h"
#include "onewire.h"
#ifdef SML_SUPPORT
#include "sml.h"
#endif /* SML_SUPPORT */

typedef enum {
	S0,
	D0,
	ONEWIRE,
	RANDOM,
#ifdef SML_SUPPORT
	SML
#endif /* SML_SUPPORT */
} meter_tag_t;

typedef struct {
	meter_tag_t tag;
	char *name;		/* short identifier for protocol */
	char *desc;		/* more detailed description */
	int periodical:1;	/* does this meter has be triggered periodically? */
} meter_type_t;

typedef struct {
	meter_type_t *type;
	char *options;
	union {
		meter_handle_s0_t s0;
		meter_handle_d0_t d0;
		meter_handle_onewire_t onewire;
		meter_handle_random_t random;
#ifdef SML_SUPPORT
		meter_handle_sml_t sml;
#endif /* SML_SUPPORT */
	} handle;
} meter_t;

/* prototypes */
void meter_init(meter_t *meter, meter_type_t *type, char *options);
void meter_free(meter_t *meter);
meter_reading_t meter_read(meter_t *meter);

int meter_open(meter_t *meter);
void meter_close(meter_t *meter);

#endif /* _METER_H_ */
