/**
 * Meter interface
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

#ifndef _METER_H_
#define _METER_H_

#include "list.h"
#include "reading.h"

using namespace std;

class Meter {

public:
	virtual ~Meter();

	virtual int open() = 0;
	virtual int close() = 0;
	virtual size_t read(reading_t *rds, size_t n);

	int getInterval();

protected:
	Meter(OptionList options);

	static int instances;
	int id;

	int interval;

	List<Channel> channels;

	pthread_t thread;
};

typedef struct {
	meter_protocol_t id;
	char *name;		/* short identifier for protocol */
	char *desc;		/* more detailed description */
	size_t max_readings;	/* how many readings can be read with 1 call */
	int periodic:1;		/* does this meter has be triggered periodically? */
} meter_details_t;

/**
 * Get list of available meter types
 */
const meter_details_t * meter_get_protocols();
const meter_details_t * meter_get_details(meter_protocol_t protocol);

#endif /* _METER_H_ */
