/**
 * Main header file
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

#ifndef _VZLOGGER_H_
#define _VZLOGGER_H_

#include <pthread.h>
#include <meter.h>

#include "config.h"

#include "list.h"

/* some hard coded configuration */
#define RETRY_PAUSE 30		/* seconds to wait after failed request */
#define BUFFER_KEEP 600		/* for the local interface; in seconds */
#define COMET_TIMEOUT 30	/* in seconds */

typedef enum {
	UNKNOWN,
	RUNNING,
	TERMINATED,
	CANCELED
} pthread_status_t;

/**
 * Type for associating channels to meters
 */
typedef struct {
	meter_t meter;
	list_t channels;
	pthread_t thread;
	pthread_status_t status;
} assoc_t;

/* Prototypes */
void print(int level, const char *format, void *id, ... );
void usage(char ** argv);

#endif /* _VZLOGGER_H_ */
