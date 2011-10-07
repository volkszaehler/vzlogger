/**
 * Wrapper to read Dallas 1-wire Sensors via the 1-wire Filesystem (owfs)
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
 
#ifndef _ONEWIRE_H_
#define _ONEWIRE_H_

#include <stdio.h>

typedef struct {
	FILE *file;
} meter_handle_onewire_t;

struct meter;	/* forward declaration */
struct reading;	/* forward declaration */

int meter_open_onewire(struct meter *mtr);
void meter_close_onewire(struct meter *mtr);
size_t meter_read_onewire(struct meter *mtr, struct reading *rds, size_t n);

#endif /* _ONEWIRE_H_ */
