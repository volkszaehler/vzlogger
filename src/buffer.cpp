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

Buffer::Buffer() : List<Reading>() {
	pthread_mutex_init(&mutex, NULL);
}

Buffer::Iterator Buffer::push(Reading data) {
	Iterator it;

	pthread_mutex_lock(&mutex);
	it = push(data);
	pthread_mutex_unlock(&mutex);

	return it;
}

void Buffer::shrink(size_t keep) {
	pthread_mutex_lock(&mutex);

//	while(size > keep && begin() != sent) {
//		pop();
//	}

	pthread_mutex_unlock(&mutex);
}

char * Buffer::dump(char *dump, size_t len) {
	size_t pos = 0;
	dump[pos++] = '{';

	for (Iterator it = begin(); it != end(); ++it) {
		if (pos < len) {
			pos += snprintf(dump+pos, len-pos, "%.2f", it->value);
		}

		/* indicate last sent reading */
		if (pos < len && it == sent) {
			dump[pos++] = '!';
		}

		/* add seperator between values */
		if (pos < len) {
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
	pthread_mutex_destroy(&mutex);
}
