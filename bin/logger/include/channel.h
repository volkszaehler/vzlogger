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
#include <meter.h>

#include "vzlogger.h"
#include "buffer.h"

typedef struct channel {
	char id[5];			/* only for internal usage & debugging */

	reading_id_t identifier;	/* channel identifier (OBIS, string) */
	reading_t last;			/* most recent reading */
	buffer_t buffer;		/* circular queue to buffer readings */

	pthread_cond_t condition;	/* pthread syncronization to notify logging thread and local webserver */
	pthread_t thread;		/* pthread for asynchronus logging */
	pthread_status_t status;	/* status of thread */

	char *middleware;		/* url to middleware */
	char *uuid;			/* unique identifier for middleware */
} channel_t;

/* prototypes */
void channel_init(channel_t *ch, const char *uuid, const char *middleware, reading_id_t identifier);
void channel_free(channel_t *ch);

#endif /* _CHANNEL_H_ */
