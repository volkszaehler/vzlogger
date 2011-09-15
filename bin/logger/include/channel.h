/**
 * Channel handling
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

#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <pthread.h>
#include <signal.h>

#include <meter.h>

#include "buffer.h"

typedef struct channel {
	unsigned int id;		/* only for internal usage & debugging */
	char *middleware;		/* url to middleware */
	char *options;			/* protocols specific configuration */
	char *uuid;			/* unique identifier for middleware */

	unsigned long interval;		/* polling interval (for sensors only) */

	meter_t meter;			/* handle to store connection status */
	buffer_t buffer;		/* circular queue to buffer readings */

	pthread_t logging_thread;	/* pthread for asynchronus logging */
	pthread_t reading_thread;	/* pthread for asynchronus reading */
	pthread_cond_t condition;	/* pthread syncronization to notify logging thread and local webserver */

	struct channel *next;		/* pointer for linked list */
} channel_t;

/* Prototypes */
void channel_init(channel_t *ch, char *uuid, char *middleware, unsigned long interval, char *options, meter_type_t *type);
void channel_free(channel_t *ch);

void * reading_thread(void *arg);

#endif /* _CHANNEL_H_ */
