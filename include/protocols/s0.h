/**
 * S0 Hutschienenzähler directly connected to an rs232 port
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
 
#ifndef _S0_H_
#define _S0_H_

#include <termios.h>

#include "meter.h"

using namespace std;

class MeterS0 : public Meter {

public:
	MeterS0(map<string, Option> options);
	virtual ~MeterS0();

	int open();
	int close();
	int read(reading_t *rds, size_t n);

protected:
	char *device;
	int resolution;

	int fd;	/* file descriptor of port */
	struct termios old_tio;	/* required to reset port */
};

#endif /* _S0_H_ */
