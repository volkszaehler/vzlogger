/**
 * Channel handling
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

#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <pthread.h>

#include "reading.h"
#include "buffer.h"

class Channel {

public:
	Channel(const char *pUuid, const char *pMiddleware, ReadingIdentifier::Ptr pIdentifier);
	virtual ~Channel();

protected:
	static int instances;
	int id;				/* only for internal usage & debugging */

	ReadingIdentifier::Ptr identifier;	/* channel identifier (OBIS, string) */
	Reading last;			/* most recent reading */
	Buffer buffer;		/* circular queue to buffer readings */

	pthread_cond_t condition;	/* pthread syncronization to notify logging thread and local webserver */
	pthread_t thread;		/* pthread for asynchronus logging */

	char *middleware;		/* url to middleware */
	char *uuid;			/* unique identifier for middleware */
};

#endif /* _CHANNEL_H_ */
