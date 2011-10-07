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
#include <math.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <curl/curl.h>

#include "vzlogger.h"
#include "list.h"
#include "channel.h"
#include "options.h"
#include "threads.h"

#ifdef LOCAL_SUPPORT
#include <microhttpd.h>
#include "local.h"
#endif /* LOCAL_SUPPORT */

list_t assocs; /* mapping between meters and channels */

extern options_t options;
extern const char *long_options_descs[];
extern const struct option long_options[];
extern const meter_type_t meter_types[];

/**
 * Print available options and some other usefull information
 */
void usage(char *argv[]) {
	const char **desc = long_options_descs;
	const struct option *op = long_options;
	const meter_type_t *type = meter_types;

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
void print(int level, const char *format, void *id, ... ) {
	va_list args;

	struct timeval now;
	struct tm * timeinfo;
	char buffer[1024], *pos = buffer;

	if (level <= options.verbose) {
		gettimeofday(&now, NULL);
		timeinfo = localtime(&now.tv_sec);
		
		pos += sprintf(pos, "[");
		pos += strftime(pos, 16, "%b %d %H:%M:%S", timeinfo);

		if (id != NULL) {
			pos += sprintf(pos, "][%s]\t", (char *) id);
		}
		else {
			pos += sprintf(pos, "]\t");
		}

		va_start(args, id);
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
	
	foreach(assocs, it) {
		assoc_t *assoc = (assoc_t *) it->data;
	
		pthread_cancel(assoc->thread);
		
		foreach(assoc->channels, it) {
			channel_t *ch = (channel_t *) it->data;
			
			pthread_cancel(ch->thread);
		}
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

	/* initialize adts and apis */
	curl_global_init(CURL_GLOBAL_ALL);
	list_init(&assocs);
	
	parse_options(argc, argv, &options);	/* parse command line arguments */
	parse_channels(options.config, &assocs);/* parse channels from configuration */

	/* open connection meters & start threads */
	foreach(assocs, it) {
		assoc_t *assoc = (assoc_t *) it->data;
		meter_t *mtr = &assoc->meter;
	
		int res = meter_open(mtr);
		if (res < 0) {
			exit(EXIT_FAILURE);
		}
		print(5, "Meter connected", mtr);
		pthread_create(&assoc->thread, NULL, &reading_thread, (void *) assoc);
		print(5, "Meter thread started", mtr);
		
		foreach(assoc->channels, it) {
			channel_t *ch = (channel_t *) it->data;
			
			/* set buffer length for perriodic meters */
			if (mtr->type->periodic && options.local) {
				ch->buffer.keep = ceil(BUFFER_KEEP / (double) ch->interval);
			}
			
			if (ch->status != RUNNING && options.logging) {
				pthread_create(&ch->thread, NULL, &logging_thread, (void *) ch);
				print(5, "Logging thread started", ch);
			}
		}
	}

#ifdef LOCAL_SUPPORT
	 /* start webserver for local interface */
	struct MHD_Daemon *httpd_handle = NULL;
	if (options.local) {
		print(5, "Starting local interface HTTPd on port %i", NULL, options.port);
		httpd_handle = MHD_start_daemon(
			MHD_USE_THREAD_PER_CONNECTION,
			options.port,
			NULL, NULL,
			&handle_request, &assocs,
			MHD_OPTION_END
		);
	}
#endif /* LOCAL_SUPPORT */

	/* wait for all threads to terminate */
	foreach(assocs, it) {
		assoc_t *assoc = (assoc_t *) it->data;
		meter_t *mtr = &assoc->meter;
		
		pthread_join(assoc->thread, NULL);
		
		foreach(assoc->channels, it) {
			channel_t *ch = (channel_t *) it->data;
			
			pthread_join(ch->thread, NULL);
			
			channel_free(ch);
		}
		
		list_free(&assoc->channels);

		meter_close(mtr); /* closing connection */
		meter_free(mtr);
	}
	
#ifdef LOCAL_SUPPORT
	/* stop webserver */
	if (httpd_handle) {
		print(8, "Stopping local interface HTTPd on port %i", NULL, options.port);
		MHD_stop_daemon(httpd_handle);
	}
#endif /* LOCAL_SUPPORT */

	/* householding */
	list_free(&assocs);
	curl_global_cleanup();
	
	print(10, "Bye bye!", NULL);
	return EXIT_SUCCESS;
}
