/**
 * Main header file
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

#ifndef _VZLOGGER_H_
#define _VZLOGGER_H_

#include "../../config.h"

#include <meter.h>

#include "channel.h"

/* some hard coded configuration */
#define RETRY_PAUSE 10 //600	/* seconds to wait after failed request */
#define BUFFER_DURATION 60	/* in seconds */
#define BUFFER_LENGTH 256	/* in readings */
#define COMET_TIMEOUT 30	/* seconds */

/* Prototypes */
void print(int level, char *format, channel_t *ch, ... );
void usage(char ** argv);

#endif /* _VZLOGGER_H_ */
