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

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../include/s0.h"

/**
 * Setup serial port
 */
int meter_s0_open(meter_handle_s0_t *handle, char *options) {
	/* open port */
	handle->fd = open(options, O_RDWR | O_NOCTTY); 

        if (handle->fd < 0) {
        	return -1;
        }

	/* save current port settings */
	tcgetattr(handle->fd, &handle->oldtio);


	/* configure port */
	struct termios tio;
	memset(&tio, 0, sizeof(struct termios));
	
	tio.c_cflag = B300 | CS8 | CLOCAL | CREAD;
        tio.c_iflag = IGNPAR;
        tio.c_oflag = 0;
        tio.c_lflag = 0;	/* set input mode (non-canonical, no echo,...) */        
        tio.c_cc[VTIME] = 0;	/* inter-character timer unused */
        tio.c_cc[VMIN] = 1; 	/* blocking read until data is received */
        
        /* apply configuration */
        tcsetattr(handle->fd, TCSANOW, &tio);
        
        return 0;
}

void meter_s0_close(meter_handle_s0_t *handle) {
	/* reset serial port */
	tcsetattr(handle->fd, TCSANOW, &handle->oldtio);

	/* close serial port */
	close(handle->fd);
}

meter_reading_t meter_s0_read(meter_handle_s0_t *handle) {
	char buf[8];
	
	meter_reading_t rd;
	
	rd.value = 1;
	
        tcflush(handle->fd, TCIOFLUSH);
	
	read(handle->fd, buf, 8); /* blocking until one character/pulse is read */
	gettimeofday(&rd.tv, NULL);
	
	/* wait some ms for debouncing */
	usleep(30000);
	
	return rd;
}

