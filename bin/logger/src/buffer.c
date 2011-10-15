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

void buffer_init(buffer_t *buf) {
	pthread_mutex_init(&buf->mutex, NULL);

	pthread_mutex_lock(&buf->mutex);
	buf->head = NULL;
	buf->tail = NULL;
	buf->sent = NULL;
	buf->size = 0;
	buf->keep = 0;
	pthread_mutex_unlock(&buf->mutex);
}

reading_t * buffer_push(buffer_t *buf, reading_t *rd) {
	reading_t *new;

	/* allocate memory for new reading */
	new = malloc(sizeof(reading_t));

	/* cannot allocate memory */
	if (new == NULL) {
		/* => delete old readings (ring buffer) */
		if (buf->size > 0) {
			new = buf->head;

			pthread_mutex_lock(&buf->mutex);
			buf->head = new->next;
			buf->size--;
			pthread_mutex_unlock(&buf->mutex);
		}
		else { /* giving up :-( */
			return NULL;
		}
	}

	memcpy(new, rd, sizeof(reading_t));

	pthread_mutex_lock(&buf->mutex);
	if (buf->size == 0) { /* empty buffer */
		buf->head = new;
	}
	else {
		buf->tail->next = new;
	}

	new->next = NULL;
	
	buf->tail = new;
	buf->size++;
	pthread_mutex_unlock(&buf->mutex);

	return new;
}

void buffer_clean(buffer_t *buf) {
	pthread_mutex_lock(&buf->mutex);
	while(buf->size > buf->keep && buf->head != buf->sent) {
		reading_t *pop = buf->head;

		buf->head = buf->head->next;
		buf->size--;

		free(pop);
	}
	pthread_mutex_unlock(&buf->mutex);
}

char * buffer_dump(buffer_t *buf, char *dump, int len) {
	strcpy(dump, "|");

	for (reading_t *rd = buf->head; rd != NULL; rd = rd->next) {
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

	reading_t *rd = buf->head;
	if (rd)	do {
		reading_t *tmp = rd;
		rd = rd->next;
		free(tmp);
	} while (rd);

	buf->head = NULL;
	buf->tail = NULL;
	buf->sent = NULL;
	buf->size = 0;
	buf->keep = 0;
}
