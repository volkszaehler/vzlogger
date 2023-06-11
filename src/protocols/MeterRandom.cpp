/**
 * Generate pseudo random data series with a random walk
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
 *
 * "connection" sets the maximum value
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "Options.hpp"
#include "protocols/MeterRandom.hpp"
#include <VZException.hpp>

MeterRandom::MeterRandom(std::list<Option> options) : Protocol("random") {
	OptionList optlist;

	_min = 0;
	_max = 40;
	_last = (_max + _min) / 2; /* start in the middle */

	try {
		_min = optlist.lookup_double(options, "min");
	} catch (vz::OptionNotFoundException &e) {
		_min = 0;
	} catch (vz::VZException &e) {
		print(log_alert, "Min value has to be a floating point number (e.g. '40.0')",
			  name().c_str());
		throw;
	}

	try {
		_max = optlist.lookup_double(options, "max");
	} catch (vz::OptionNotFoundException &e) {
		_max = 0;
	} catch (vz::VZException &e) {
		print(log_alert, "Max value has to be a floating point number (e.g. '40.0')",
			  name().c_str());
		throw;
	}
}

MeterRandom::~MeterRandom() {}

int MeterRandom::open() {

	// TODO rewrite to use /dev/u?random
	srand(time(NULL)); /* initialize PNRG */

	return SUCCESS; /* can't fail */
}

int MeterRandom::close() { return SUCCESS; }

ssize_t MeterRandom::read(std::vector<Reading> &rds, size_t n) {
	if (rds.size() < 1)
		return -1;

	double step = ltqnorm((float)rand() / RAND_MAX);
	double newval = _last + step;

	/* check boundaries */
	_last += (newval > _max || newval < _min) ? -step : step;

	rds[0].value(_last);
	rds[0].time();
	rds[0].identifier(new NilIdentifier());

	return 1;
}
