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

Buffer::Buffer() {
	pthread_mutex_init(&_mutex, NULL);

	pthread_mutex_lock(&_mutex);
	//buf->head = NULL;
	//buf->tail = NULL;
	//buf->sent = NULL;
	//buf->size = 0;
	_keep = 0;
	pthread_mutex_unlock(&_mutex);
}

void Buffer::push(const Reading &rd) {
		pthread_mutex_lock(&_mutex);
	  _sent.push_back(rd);
		pthread_mutex_unlock(&_mutex);
	  
#if 0
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
#endif
}

void Buffer::clean() {

	lock();
  for(iterator it = _sent.begin(); it!= _sent.end(); it++) {
    if(it->deleted()) _sent.erase(it);
  }
  
  _sent.clear();
#if 0
	while(buf->size > buf->keep && buf->head != buf->sent) {
		reading_t *pop = buf->head;

		buf->head = buf->head->next;
		buf->size--;

		free(pop);
	}
#endif
  unlock();
}

char * Buffer::dump(char *dump, size_t len) {
	size_t pos = 0;
	dump[pos++] = '{';

	//for (Reading *rd = buf->head; rd != NULL; rd = rd->next) {
  for(const_iterator it = _sent.begin(); it!= _sent.end(); it++) {
    if (pos < len) {
			pos += snprintf(dump+pos, len-pos, "%.2f", it->value());
		}

		/* indicate last sent reading */
		if (pos < len && _sent.end() == it) {
			dump[pos++] = '!';
		}

		/* add seperator between values */
		if (pos < len && it != _sent.end()) {
			dump[pos++] = ',';
		}
	}

	if (pos+1 < len) {
		dump[pos++] = '}';
		dump[pos] = '\0'; /* zero terminated string */
	}

	return (pos < len) ? dump : NULL; /* buffer full? */
}

Buffer::~Buffer() {
	pthread_mutex_destroy(&_mutex);

#if 0
	reading_t *rd = buf->head;
	if (rd)	do {
		reading_t *tmp = rd;
		rd = rd->next;
		free(tmp);
	} while (rd);

	//buf->head = NULL;
	//buf->tail = NULL;
	buf->sent = NULL;
#endif
	//buf->size = 0;
	_keep = 0;
}
