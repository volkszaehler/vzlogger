/**
 * Channel class
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "vzlogger.h"
#include "api.h"
#include "channel.h"
#include "options.h"

extern options_t opts;

void * reading_thread(void *arg) {
	channel_t *ch = (channel_t *) arg;

	do { /* start thread main loop */
		meter_reading_t rd, *added;
		
		rd = meter_read(&ch->meter);
		print(1, "Value read: %.2f at %.0f", ch, rd.value, api_tvtof(rd.tv));

		/* add reading to buffer */
		added = buffer_push(&ch->buffer, rd);
		
		if ( /* queue reading into sending queue if... */
			added				/* we got it in the buffer */
			&& ch->buffer.sent == NULL	/* there are no other readings pending in queue */
			&& (opts.daemon || !opts.local)	/* we aren't in local only mode */
		) {
			ch->buffer.sent = added;
		}
		
		/* shrink buffer */
		buffer_clean(&ch->buffer);
		
		pthread_mutex_lock(&ch->buffer.mutex);
		pthread_cond_broadcast(&ch->condition); /* notify webserver and logging thread */
		pthread_mutex_unlock(&ch->buffer.mutex);

		/* Debugging */
		if (opts.verbose >= 10) {
			char dump[1024];
			buffer_dump(&ch->buffer, dump, 1024);
			print(10, "Buffer dump: %s (size=%i, memory=%i)", ch, dump, ch->buffer.size, ch->buffer.keep);
		}

		if ((opts.daemon || opts.local) && ch->meter.type->periodical) {
			print(8, "Next reading in %i seconds", ch, ch->interval);
			sleep(ch->interval);
		}
	} while (opts.daemon || opts.local);
	
	return NULL;
}

void channel_init(channel_t *ch, char *uuid, char *middleware, unsigned long interval, char *options, meter_type_t *type) {
	static int instances; /* static to generate channel ids */
	
	int keep; /* number of readings to cache for local interface */
	if (opts.local) {
		if (type->periodical) {
			keep = (BUFFER_DURATION / interval) + 1; /* determine cache length by interval */
		}
		else {
			keep = BUFFER_LENGTH; /* fixed cache length for meters */
		}
	}
	else {
		keep = 0; /* don't cache if we have no HTTPd started */
	}

	ch->id = instances++;
	ch->interval = interval;
	ch->uuid = strdup(uuid);
	ch->middleware = strdup(middleware);
	ch->options = strdup(options);

	meter_init(&ch->meter, type, options);
	buffer_init(&ch->buffer, keep); /* initialize buffer */

	pthread_cond_init(&ch->condition, NULL); /* initialize thread syncronization helpers */
}

/**
 * Free all allocated memory recursivly
 */
void channel_free(channel_t *ch) {
	buffer_free(&ch->buffer);
	meter_free(&ch->meter);
	pthread_cond_destroy(&ch->condition);
	
	free(ch->uuid);
	free(ch->options);
	free(ch->middleware);
}
