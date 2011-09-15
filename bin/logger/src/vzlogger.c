/**
 * Main source file
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


#include <stdio.h> /* for print() */
#include <stdarg.h>

#include <stdlib.h>
#include <signal.h>
#include <getopt.h>

#include "vzlogger.h"

#include "list.h"
#include "buffer.h"
#include "channel.h"
#include "api.h"
#include "options.h"

#ifdef LOCAL_SUPPORT
#include <microhttpd.h>
#include "local.h"
#endif /* LOCAL_SUPPORT */

/* global variables */
list_t chans;
extern const options_t opts;
extern const char *long_options_descs[];
extern const struct option long_options[];
extern const meter_type_t meter_types[];

/**
 * Print available options and some other usefull information
 */
void usage(char *argv[]) {
	char **desc = long_options_descs;
	struct option *op = long_options;
	meter_type_t *type = meter_types;

	printf("Usage: %s [options]\n\n", argv[0]);
	printf("  following options are available:\n");

	while (op->name && *desc) {
		printf("\t-%c, --%-12s\t%s\n", op->val, op->name, *desc);
		op++; desc++;
	}

	printf("\n");
	printf("  following protocol types are supported:\n");

	while (type->name) {
		printf("\t%-12s\t%s\n", type->name, type->desc);
		type++;
	}

	printf("\n%s - volkszaehler.org logging utility\n", PACKAGE_STRING);
	printf("by Steffen Vogel <stv0g@0l.de>\n");
	printf("send bugreports to %s\n", PACKAGE_BUGREPORT);
}

/**
 * Wrapper to log notices and errors
 *
 * @param ch could be NULL for general messages
 * @todo integrate into syslog
 */
void print(int level, char * format, channel_t *ch, ... ) {
	va_list args;

	struct timeval now;
	struct tm * timeinfo;
	char buffer[1024] = "[", *pos = buffer+1;

	if (level <= opts.verbose) {
		gettimeofday(&now, NULL);
		timeinfo = localtime(&now.tv_sec);
		pos += strftime(pos, 16, "%b %d %H:%M:%S", timeinfo);

		pos += sprintf(pos, ".%03lu] ", now.tv_usec / 1000);

		if (ch != NULL) {
			pos += sprintf(pos, "[ch#%i] ", ch->id);
		}

		va_start(args, ch);
		pos += vsprintf(pos, format, args);
		va_end(args);

		fprintf((level > 0) ? stdout : stderr, "%s\n", buffer);
	}
}

/**
 * Cancel threads
 *
 * Threads gets joined in main()
 */
void quit(int sig) {
	print(2, "Closing connections to terminate", NULL);
	for (channel_t *ch = chans.start; ch != NULL; ch = ch->next) {
		pthread_cancel(ch->logging_thread);
		pthread_cancel(ch->reading_thread);
	}
}

/**
 * The main loop
 */
int main(int argc, char *argv[]) {
	/* bind signal handler */
	struct sigaction action;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	action.sa_handler = quit;
	sigaction(SIGINT, &action, NULL);

	list_init(&chans);
	parse_options(argc, argv, &opts); /* parse command line arguments */
	parse_channels(opts.config, &chans); /* parse channels from configuration */

	curl_global_init(CURL_GLOBAL_ALL); /* global intialization for all threads */

	for (channel_t *ch = chans.start; ch != NULL; ch = ch->next) {
		print(5, "Opening connection to meter", ch);
		meter_open(&ch->meter);
		
		print(5, "Starting threads", ch);
		pthread_create(&ch->logging_thread, NULL, &logging_thread, (void *) ch);
		pthread_create(&ch->reading_thread, NULL, &reading_thread, (void *) ch);
	}

#ifdef LOCAL_SUPPORT
	 /* start webserver for local interface */
	struct MHD_Daemon *httpd_handle = NULL;
	if (opts.local) {
		print(5, "Starting local interface HTTPd on port %i", NULL, opts.port);
		httpd_handle = MHD_start_daemon(
			MHD_USE_THREAD_PER_CONNECTION,
			opts.port,
			NULL, NULL,
			handle_request,
			NULL,
			MHD_OPTION_END
		);
	}
#endif /* LOCAL_SUPPORT */

	/* wait for all threads to terminate */
	for (channel_t *ch = chans.start; ch != NULL; ch = ch->next) {
		pthread_join(ch->logging_thread, NULL);
		pthread_join(ch->reading_thread, NULL);

		meter_close(&ch->meter); /* closing connection */
	}

#ifdef LOCAL_SUPPORT
	/* stop webserver */
	if (httpd_handle) {
		print(8, "Stopping local interface HTTPd on port %i", NULL, opts.port);
		MHD_stop_daemon(httpd_handle);
	}
#endif /* LOCAL_SUPPORT */

	/* householding */
	list_free(&chans);
	curl_global_cleanup();
	
	print(10, "Bye bye!", NULL);

	return EXIT_SUCCESS;
}
