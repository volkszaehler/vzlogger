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

#include <pthread.h>

#include "../config.h" /* GNU buildsystem config */

#include "config.h"
#include "meter.h"
#include "common.h"
#include "list.h"

/* enumerations */
typedef enum {
	status_unknown,
	status_running,
	status_terminated,
	status__cancelled
} pthread_status_t;

/**
 * Type for mapping channels to meters
 */
typedef struct map {
	meter_t meter;
	list_t channels;

	pthread_t thread;
	pthread_status_t status;
} map_t;

/* prototypes */
void quit(int sig);
void daemonize();

void show_usage(char ** argv);
void show_aliases();

int options_parse(int argc, char *argv[], config_options_t *options);

#endif /* _VZLOGGER_H_ */
