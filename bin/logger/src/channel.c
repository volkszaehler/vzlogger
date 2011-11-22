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
#include <stdio.h>

#include "channel.h"

void channel_init(channel_t *ch, const char *uuid, const char *middleware, reading_id_t identifier, int counter) {
	static int instances; /* static to generate channel ids */
	snprintf(ch->id, 5, "ch%i", instances++);

	ch->identifier = identifier;
	ch->status = status_unknown;
	ch->counter = counter;
	ch->last = 0;

	ch->uuid = strdup(uuid);
	ch->middleware = strdup(middleware);

	buffer_init(&ch->buffer); /* initialize buffer */
	pthread_cond_init(&ch->condition, NULL); /* initialize thread syncronization helpers */
}

/**
 * Free all allocated memory recursivly
 */
void channel_free(channel_t *ch) {
	buffer_free(&ch->buffer);
	pthread_cond_destroy(&ch->condition);

	free(ch->uuid);
	free(ch->middleware);
}

reading_t * channel_add_readings(channel_t *ch, meter_protocol_t protocol, reading_t *rds, size_t n) {
	reading_t *first = NULL; /* first unsent reading which has been added */

	double delta;

	for (int i = 0; i < n; i++) {
		int add = FALSE;
		if (protocol == meter_protocol_d0 || protocol == meter_protocol_sml) { /* intelligent protocols require matching ids */
			if (obis_compare(rds[i].identifier.obis, ch->identifier.obis) == 0) {
				add = TRUE;
			}
		}
		else { /* no channel identifier, adding all readings to buffer */
			add = TRUE;
		}

		if (add) {
			delta = (ch->last == 0) ? 0 : rds[i].value - ch->last; /* ignoring first reading when no preceeding reading is available (no consumption) */
			ch->last = rds[i].value;

			print(log_info, "Adding reading to queue (value=%.2f delta=%.2f ts=%.3f)", ch, rds[i].value, delta, tvtod(rds[i].time));

			if (ch->counter) { /* send relative consumption since last reading */
				rds[i].value = delta;
			}

			reading_t *added = buffer_push(&ch->buffer, &rds[i]); /* remember last value to calculate relative consumption */

			if (first == NULL) {
				first = added;
			} 
		}
	}

	return first;
}
