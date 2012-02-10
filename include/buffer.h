/**
 * Circular buffer (double linked, threadsafe)
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

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <pthread.h>
#include <sys/time.h>

#include "meter.h"
#include "list.h"

class Buffer : List<Reading> {

public:
	Buffer(size_t keep);
	virtual ~Buffer();

	shrink(size_t keep = 0);
	dump(char *dump, size_t len);

protected:
	ListIterator<Reading> *sent;

	size_t keep;	/* number of readings to cache for local interface */

	pthread_mutex_t mutex;
};

#endif /* _BUFFER_H_ */

