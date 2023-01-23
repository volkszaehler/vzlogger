/**
 * Circular buffer (double linked)
 *
 * Used to store recent readings and buffer in case of net inconnectivity
 *
 * @author Steffen Vogel <info@steffenvogel.de>
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
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

#include "common.h"
#include "float.h" /* double min max */
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Buffer.hpp"

Buffer::Buffer() : _keep(32), _last_avg(0) {
	_newValues = false;
	pthread_mutex_init(&_mutex, NULL);
	_aggmode = NONE;
}

void Buffer::push(const Reading &rd) {
	lock();
	_sent.push_back(rd);
	unlock();
}

void Buffer::aggregate(int aggtime, bool aggFixedInterval) {
	if (_aggmode == NONE)
		return;

	lock();
	if (_aggmode == MAX) {
		Reading *latest = NULL;
		double aggvalue = -DBL_MAX;
		for (iterator it = _sent.begin(); it != _sent.end(); it++) {
			if (!it->deleted()) {
				if (!latest) {
					latest = &*it;
				} else {
					if (it->time_ms() > latest->time_ms()) {
						latest = &*it;
					}
				}
				aggvalue = std::max(aggvalue, it->value());
				print(log_debug, "%f @ %lld", "MAX", it->value(), it->time_ms());
			}
		}
		for (iterator it = _sent.begin(); it != _sent.end(); it++) {
			if (!it->deleted()) {
				if (&*it == latest) {
					it->value(aggvalue);
					print(log_debug, "RESULT %f @ %lld", "MAX", it->value(), it->time_ms());
				} else {
					it->mark_delete();
				}
			}
		}
	} else if (_aggmode == AVG) {
		// AVG needs to handle tuples with different distances properly:
		// so we need to consider the last tuple from last aggregate call as well
		// and use this value as the starting point.
		// we assume buffer values are already sorted by time here! TODO: is this always true?
		// otherwise a sort by time would be needed but this might conflict with _last_avg

		Reading *latest = NULL;
		double aggvalue = 0;
		double aggtimespan = 0.0;
		unsigned int aggcount = 0;
		Reading *previous = _last_avg;
		for (iterator it = _sent.begin(); it != _sent.end(); it++) {
			if (!it->deleted()) {
				if (!latest) {
					latest = &*it;
				} else {
					if (it->time_ms() > latest->time_ms()) {
						latest = &*it;
					}
				}
				print(log_debug, "[%d] %f @ %lld", "AVG", aggcount, it->value(), it->time_ms());
				if (previous) {
					double timespan = ((double)(it->time_ms() - previous->time_ms())) / 1000.0;
					aggvalue += previous->value() * timespan; // timespan between prev. and this one
					aggtimespan += timespan;
				}
				previous = &*it;
				aggcount++;
			}
		}
		for (iterator it = _sent.begin(); it != _sent.end(); it++) {
			if (!it->deleted()) {
				if (&*it == latest) {
					// store the last value before we modify it.
					if (!_last_avg)
						_last_avg = new Reading(*it);
					else
						*_last_avg = *it;

					if (aggtimespan > 0.0)
						it->value(aggvalue / aggtimespan);
					// else keep current value (if no previous and just single value)
					print(log_debug, "[%d] RESULT %f @ %lld", "AVG", aggcount, it->value(),
						  it->time_ms());
				} else {
					it->mark_delete();
				}
			}
		}
	} else if (_aggmode == SUM) {
		Reading *latest = NULL;
		double aggvalue = 0;
		for (iterator it = _sent.begin(); it != _sent.end(); it++) {
			if (!it->deleted()) {
				if (!latest) {
					latest = &*it;
				} else {
					if (it->time_ms() > latest->time_ms()) {
						latest = &*it;
					}
				}
				aggvalue = aggvalue + it->value();
				print(log_debug, "%f @ %lld", "SUM", it->value(), it->time_ms());
			}
		}
		for (iterator it = _sent.begin(); it != _sent.end(); it++) {
			if (!it->deleted()) {
				if (&*it == latest) {
					it->value(aggvalue);
					print(log_debug, "RESULT %f @ %lld", "SUM", it->value(), it->time_ms());
				} else {
					it->mark_delete();
				}
			}
		}
	}
	/* fix timestamp if aggFixedInterval set */
	if ((aggFixedInterval == true) && (aggtime > 0)) {
		struct timeval tv;
		for (iterator it = _sent.begin(); it != _sent.end(); it++) {
			if (!it->deleted()) {
				tv.tv_usec = 0;
				tv.tv_sec = aggtime * (long int)((it->time_ms() / 1000) / aggtime);
				it->time(tv);
			}
		}
	}
	unlock();
	clean();
	return;
}

void Buffer::clean(bool deleted_only) {
	lock();
	if (deleted_only) {
		for (iterator it = _sent.begin(); it != _sent.end(); it++) {
			if (it->deleted()) {
				it = _sent.erase(it);
				it--;
			}
		}
	} else {
		_sent.clear();
	}
	unlock();
}

void Buffer::undelete() {
	lock();
	for (iterator it = _sent.begin(); it != _sent.end(); it++) {
		it->reset();
	}
	unlock();
}

std::string Buffer::dump() {
	std::ostringstream o;
	o << '{';

	lock();
	o << std::setprecision(4);
	for (iterator it = _sent.begin(); it != _sent.end(); it++) {
		o << it->value();

		/* indicate last sent reading */
		if (_sent.end() == it) {
			o << '!';
		} else {
			/* add seperator between values */
			o << ',';
		}
	}
	o << '}';
	unlock();
	return o.str();
}

Buffer::~Buffer() {
	pthread_mutex_destroy(&_mutex);
	if (_last_avg)
		delete _last_avg;
}
