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
#include <meter.h>

#include "config.h"
#include "list.h"

enum {
	LOG_ERROR = -1,
	LOG_STD = 0,
	LOG_INFO = 5,
	LOG_DEBUG = 10,
	LOG_FINEST = 15
};

typedef enum {
	UNKNOWN,
	RUNNING,
	TERMINATED,
	CANCELED
} pthread_status_t;

/**
 * Type for associating channels to meters
 */
typedef struct {
	meter_t meter;
	list_t channels;
	int interval;

	pthread_t thread;
	pthread_status_t status;
} assoc_t;

/**
 * Options from command line
 */
typedef struct {
	char *config;		/* filename of configuration */
	char *log;		/* filename for logging */
	FILE *logfd;

	int port;		/* TCP port for local interface */
	int verbosity;		/* verbosity level */
	int comet_timeout;	/* in seconds;  */
	int buffer_length;	/* in seconds; how long to buffer readings for local interfalce */
	int retry_pause;	/* in seconds; how long to pause after an unsuccessful HTTP request */

	/* boolean bitfields, padding at the end of struct */
	int channel_index:1;	/* give a index of all available channels via local interface */
	int daemon:1;		/* run in background */
	int foreground:1;	/* dont fork in background */
	int local:1;		/* enable local interface */
	int logging:1;		/* start logging threads, depends on local & daemon */
} options_t;

/* Prototypes */
void print(int level, const char *format, void *id, ... );
void usage(char ** argv);
void quit(int sig);
void parse_options(int argc, char *argv[], options_t *options);

#endif /* _VZLOGGER_H_ */
