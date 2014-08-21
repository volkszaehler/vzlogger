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
#include "float.h" /* double min max */
#include "common.h"

#include "Buffer.hpp"

Buffer::Buffer() :
		_keep(32)
{
	_newValues=false;
	pthread_mutex_init(&_mutex, NULL);
	_aggmode=NONE;
}

void Buffer::push(const Reading &rd) {
	lock();
	_sent.push_back(rd);
	unlock();
}

void Buffer::aggregate(int aggtime, bool aggFixedInterval) {
	if (_aggmode == NONE) return;

	lock();
	if (_aggmode == MAXIMUM) {
		Reading *latest=NULL;
		double aggvalue=DBL_MIN;
		for (iterator it = _sent.begin(); it!= _sent.end(); it++) {
			if (! it->deleted()) {
				if (!latest) {
					latest=&*it;
				} else {
					if (it->tvtod() > latest->tvtod()) {
						latest=&*it;
					}
				}
				aggvalue=std::max(aggvalue,it->value());
				print(log_debug, "%f @ %f", "MAX",it->value(),it->tvtod());
			}
		}
		for (iterator it = _sent.begin(); it!= _sent.end(); it++) {
			if (! it->deleted()) {
				if (&*it==latest) {
					it->value(aggvalue);
					print(log_debug, "RESULT %f @ %f", "MAX",it->value(),it->tvtod());
				} else {
					it->mark_delete();
				}
			}
		}
	} else if (_aggmode == AVG) {
		Reading *latest=NULL;
		double aggvalue=0;
		int aggcount=0;
		for (iterator it = _sent.begin(); it!= _sent.end(); it++) {
			if (! it->deleted()) {
				if (!latest) {
					latest=&*it;
				} else {
					if (it->tvtod() > latest->tvtod()) {
						latest=&*it;
					}
				}
				aggvalue=aggvalue+it->value();
				print(log_debug, "[%d] %f @ %f", "AVG",aggcount,it->value(),it->tvtod());

				aggcount++;
			}
		}
		for (iterator it = _sent.begin(); it!= _sent.end(); it++) {
			if (! it->deleted()) {
				if (&*it==latest) {
					it->value(aggvalue/aggcount);
					print(log_debug, "[%d] RESULT %f @ %f", "AVG",aggcount,it->value(),it->tvtod());
				} else {
					it->mark_delete();
				}
			}
		}
	} else if (_aggmode == SUM) {
		Reading *latest=NULL;
		double aggvalue=0;
		for (iterator it = _sent.begin(); it!= _sent.end(); it++) {
			if (! it->deleted()) {
				if (!latest) {
					latest=&*it;
				} else {
					if (it->tvtod() > latest->tvtod()) {
						latest=&*it;
					}
				}
				aggvalue=aggvalue+it->value();
				print(log_debug, "%f @ %f", "SUM",it->value(),it->tvtod());
			}
		}
		for (iterator it = _sent.begin(); it!= _sent.end(); it++) {
			if (! it->deleted()) {
				if (&*it==latest) {
					it->value(aggvalue);
					print(log_debug, "RESULT %f @ %f", "SUM",it->value(),it->tvtod());
				} else {
					it->mark_delete();
				}
			}
		}
	}
	/* fix timestamp if aggFixedInterval set */
	if ((aggFixedInterval==true) && (aggtime>0)) {
		struct timeval tv;
		for (iterator it = _sent.begin(); it!= _sent.end(); it++) {
			if (! it->deleted()) {
				tv.tv_usec = 0;
				tv.tv_sec = aggtime * (long int)(it->tvtod() / aggtime);
				it->time(tv);
			}
		}
	}
	unlock();
	clean();
	return;
}


void Buffer::clean() {
	lock();
	for (iterator it = _sent.begin(); it!= _sent.end(); it++) {
		if (it->deleted()) {
			it = _sent.erase(it);
			it--;
		}

	}
//_sent.clear();
	unlock();
}

void Buffer::undelete() {
	lock();
	for (iterator it = _sent.begin(); it!= _sent.end(); it++) {
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
	for (iterator it = _sent.begin(); it!= _sent.end(); it++) {
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
