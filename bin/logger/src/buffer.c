/**
 * Circular buffer (double linked)
 *
 * Used to store recent readings and buffer in case of net inconnectivity
 *
 * @author Steffen Vogel <info@steffenvogel.de>
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @package vzlogger
 * @license http://opensource.org/licenses/gpl-license.php GNU Public License
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
#include <stdio.h>
#include <string.h>

#include "buffer.h"

void buffer_init(buffer_t *buf, int keep) {
	pthread_mutex_init(&buf->mutex, NULL);

	pthread_mutex_lock(&buf->mutex);
	buf->last = NULL;
	buf->start = NULL;
	buf->sent = NULL;
	buf->size = 0;
	buf->keep = keep;
	pthread_mutex_unlock(&buf->mutex);
}

meter_reading_t * buffer_push(buffer_t *buf, meter_reading_t rd) {
	meter_reading_t *new;
	
	/* allocate memory for new reading */
	new = malloc(sizeof(meter_reading_t));

	/* cannot allocate memory */
	if (new == NULL) {
		/* => delete old readings (ring buffer) */
		if (buf->size > 0) {
			pthread_mutex_lock(&buf->mutex);	
			new = buf->start;
			buf->start = new->next;
			buf->size--;
			pthread_mutex_unlock(&buf->mutex);
		}
		else { /* giving up :-( */
			return NULL;
		}
	}

	memcpy(new, &rd, sizeof(meter_reading_t));

	pthread_mutex_lock(&buf->mutex);
	if (buf->size == 0) { /* empty buffer */
		buf->start = new;
	}
	else {
		buf->last->next = new;
	}

	new->next = NULL;
	
	buf->last = new;
	buf->size++;
	
	pthread_mutex_unlock(&buf->mutex);

	return new;
}

void buffer_clean(buffer_t *buf) {
	pthread_mutex_lock(&buf->mutex);
	while(buf->size > buf->keep && buf->start != buf->sent) {
		meter_reading_t *pop = buf->start;

		buf->start = buf->start->next;
		buf->size--;

		free(pop);
	}
	pthread_mutex_unlock(&buf->mutex);
}

char * buffer_dump(buffer_t *buf, char *dump, int len) {
	strcpy(dump, "|");

	for (meter_reading_t *rd = buf->start; rd != NULL; rd = rd->next) {
		char tmp[16];
		sprintf(tmp, "%.2f|", rd->value);

		if (strlen(dump)+strlen(tmp) < len) {
			if (buf->sent == rd) { /* indicate last sent reading */
				strcat(dump, "!");
			}

			strcat(dump, tmp);
		}
		else {
			return NULL; /* dump buffer is full! */
		}
	}

	return dump;
}

void buffer_free(buffer_t *buf) {
	pthread_mutex_destroy(&buf->mutex);

	meter_reading_t *rd = buf->start;
	do {
		meter_reading_t *tmp = rd;
		rd = rd->next;
		free(tmp);
	} while (rd);

	memset(buf, 0, sizeof(buffer_t));
}
