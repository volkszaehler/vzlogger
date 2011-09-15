/**
 * Parsing commandline options and channel list
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

#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include "list.h"

/**
 * Options from command line
 */
typedef struct {
	char *config;		/* path to config file */
	unsigned int port;	/* tcp port for local interface */
	int verbose;		/* verbosity level */

	/* boolean bitfields, at the end of struct */
	int daemon:1;
	int local:1;		/* enable local interface */
} options_t;

/* Prototypes */
void parse_options(int argc, char *argv[], options_t *opts);
void parse_channels(char *filename, list_t *chans);

#endif /* _OPTIONS_H_ */
