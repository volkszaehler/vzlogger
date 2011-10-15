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

#include <sys/socket.h>
#include <sys/time.h>

#include "config.h"
#include "obis.h"

/* meter types */
#include "random.h"
#include "s0.h"
#include "d0.h"
#include "onewire.h"
#ifdef SML_SUPPORT
#include "sml.h"
#endif /* SML_SUPPORT */

typedef union reading_id {
	obis_id_t obis;
} reading_id_t;

typedef struct reading {
	float value;
	struct timeval time;
	union reading_id identifier;
	
	struct reading *next; /* pointer for linked list */
} reading_t;

typedef struct {
	enum {
		RANDOM,
		S0,
		D0,
		ONEWIRE,
		SML
	} id;
	const char *name;	/* short identifier for protocol */
	const char *desc;	/* more detailed description */
	size_t max_readings;	/* how many readings can be read with 1 call */
	int periodic:1;		/* does this meter has be triggered periodically? */
} meter_type_t;

typedef struct meter {
	char id[5];		/* only for internal usage & debugging */
	char *connection;	/* args for connection, further configuration etc */
	meter_type_t *type;
		
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

/* Prototypes */

/**
 * Converts timeval structure to double
 *
 * @param tv the timeval structure
 * @return the double value
 */
double tvtod(struct timeval tv);

/**
 * Initialize meter
 *
 * @param mtr the meter structure to initialze
 * @param type the type it should be initialized with
 * @param connection type specific initialization arguments (connection settings, port, etc..)
 */
void meter_init(meter_t *mtr, const meter_type_t *type, const char *connection);

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
 */
int meter_open(meter_t *mtr);

/**
 * Dispatcher for closing meters of diffrent types
 * 
 * Reset ports, shutdown meter etc.
 *
 * @param mtr the meter structure
 */
void meter_close(meter_t *mtr);

#endif /* _METER_H_ */
