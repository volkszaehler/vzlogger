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

#include "Buffer.hpp"

Buffer::Buffer() :
		_keep(32)
{
	pthread_mutex_init(&_mutex, NULL);
}

void Buffer::push(const Reading &rd) {
	lock();
	have_newValues();
	_sent.push_back(rd);
	unlock();
}

void Buffer::clean() {
	lock();
	for(iterator it = _sent.begin(); it!= _sent.end(); it++) {
		if(it->deleted()) {
			if( it == _sent.begin() ){
				_sent.erase(it);
				it = _sent.begin();
			} else {
				it = _sent.erase(it);
			}
		}

	}
//_sent.clear();
	unlock();
}

void Buffer::undelete() {
	lock();
	for(iterator it = _sent.begin(); it!= _sent.end(); it++) {
		it->reset();
	}
	unlock();
}


void Buffer::shrink(/*size_t keep*/) {
	lock();

//	while(size > keep && begin() != sent) {
//		pop();
//	}

	unlock();
}

char * Buffer::dump(char *dump, size_t len) {
	size_t pos = 0;
	dump[pos++] = '{';

	lock();
	for(iterator it = _sent.begin(); it!= _sent.end(); it++) {
		if (pos < len) {
			pos += snprintf(dump+pos, len-pos, "%.4f", it->value());
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
	unlock();
	
	return (pos < len) ? dump : NULL; /* buffer full? */
}

Buffer::~Buffer() {
	pthread_mutex_destroy(&_mutex);
}

/*
 * Local variables:
 *  tab-width: 2
 *  c-indent-level: 2
 *  c-basic-offset: 2
 *  project-name: vzlogger
 * End:
 */
