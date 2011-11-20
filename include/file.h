/**
 * Read data from files & fifos
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
 
#ifndef _FILE_H_
#define _FILE_H_

#include <stdio.h>

typedef struct {
	char *path;
	char *regex;

	FILE *fd;
} meter_handle_file_t;

/* forward declarations */
struct meter;
struct reading;

int meter_init_file(struct meter *mtr, list_t options);
int meter_open_file(struct meter *mtr);
int meter_close_file(struct meter *mtr);
size_t meter_read_file(struct meter *mtr, struct reading *rds, size_t n);

#endif /* _FILE_H_ */
