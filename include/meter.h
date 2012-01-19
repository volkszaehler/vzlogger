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
 * - sensors:	a readout is triggered in equidistant intervals by calling
 *		the read function with an POSIX timer.
 *		The interval is set in the configuration.
 * - meters:	the meter itselfs triggers a readout.
 *		The pointer to the read function shoul be NULL.
 *		The 'interval' column in the configuration as no meaning.
 */

#include <config.h>

#include "common.h"
#include "list.h"
#include "reading.h"

/* meter protocols */
#include "protocols/file.h"
#include "protocols/exec.h"
#include "protocols/random.h"
#include "protocols/s0.h"
#include "protocols/d0.h"
#include "protocols/fluksov2.h"
#ifdef SML_SUPPORT
#include "protocols/sml.h"
#endif /* SML_SUPPORT */

typedef enum meter_procotol {
	meter_protocol_file = 1,
	meter_protocol_exec,
	meter_protocol_random,
	meter_protocol_s0,
	meter_protocol_d0,
	meter_protocol_sml,
	meter_protocol_fluksov2
} meter_protocol_t;

typedef struct meter {
	char id[5];
	int interval;

	meter_protocol_t protocol;

	union {
		meter_handle_file_t file;
		meter_handle_exec_t exec;
		meter_handle_random_t random;
		meter_handle_s0_t s0;
		meter_handle_d0_t d0;
		meter_handle_fluksov2_t fluksov2;
#ifdef SML_SUPPORT
		meter_handle_sml_t sml;
#endif /* SML_SUPPORT */
	} handle;
} meter_t;

typedef struct {
	meter_protocol_t id;
	char *name;		/* short identifier for protocol */
	char *desc;		/* more detailed description */
	size_t max_readings;	/* how many readings can be read with 1 call */
	int periodic:1;		/* does this meter has be triggered periodically? */

	/* function pointers */
	int (*init_func)(meter_t *mtr, list_t options);
	int (*open_func)(meter_t *mtr);
	int (*close_func)(meter_t *mtr);
	size_t (*read_func)(meter_t *mtr, reading_t *rds, size_t n);
} meter_details_t;

/* prototypes */

/**
 * Get list of available meter types
 */
const meter_details_t * meter_get_protocols();
const meter_details_t * meter_get_details(meter_protocol_t protocol);

/**
 * Initialize meter
 *
 * @param mtr the meter structure to initialze
 * @param list of key, 	value pairs of options
 * @return 0 on success, -1 on error
 */
int meter_init(meter_t *mtr, list_t options);

/**
 * Freeing all memory which has been allocated during the initialization
 *
 * @param mtr the meter structure
 */
void meter_free(meter_t *mtr);

/**
 * Dispatcher for blocking read from meters of diffrent types
 *
 * rds has to point to an array with space for at least n readings!
 *
 * @param mtr the meter structure
 * @param rds the array to store the readings to
 * @param n the size of the array
 * @return number of readings
 */
size_t meter_read(meter_t *mtr, reading_t rds[], size_t n);

/**
 * Dispatcher for opening meters of diffrent types,
 * 
 * Establish connection, initialize meter etc.
 *
 * @param mtr the meter structure
 * @return 0 on success, -1 on error
 */
int meter_open(meter_t *mtr);

/**
 * Dispatcher for closing meters of diffrent types
 * 
 * Reset ports, shutdown meter etc.
 *
 * @param mtr the meter structure
 * @return 0 on success, -1 on error
 */
int meter_close(meter_t *mtr);

#endif /* _METER_H_ */
