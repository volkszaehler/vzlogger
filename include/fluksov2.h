/**
 * Replacement for fluksod by directly parsing the fluksometers SPI output
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
 
#ifndef _FLUKSOV2_H_
#define _FLUKSOV2_H_

typedef struct {
	char *fifo;

	int fd;	/* file descriptor of fifo */
} meter_handle_fluksov2_t;

/* forward declarations */
struct meter;
struct reading;

int meter_init_fluksov2(struct meter *mtr, list_t options);
int meter_open_fluksov2(struct meter *mtr);
int meter_close_fluksov2(struct meter *mtr);
size_t meter_read_fluksov2(struct meter *mtr, struct reading *rds, size_t n);

#endif /* _FLUKSOV2_H_ */
