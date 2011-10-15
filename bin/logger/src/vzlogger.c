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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "vzlogger.h"
#include "list.h"
#include "channel.h"
#include "configuration.h"
#include "threads.h"

#ifdef LOCAL_SUPPORT
#include <microhttpd.h>
#include "local.h"
#endif /* LOCAL_SUPPORT */

extern const meter_type_t meter_types[];

list_t assocs;		/* mapping between meters and channels */
options_t options;	/* global application options */

/**
 * Command line options
 */
const struct option long_options[] = {
	{"config", 	required_argument,	0,	'c'},
	{"log",		required_argument,	0,	'o'},
	{"daemon", 	required_argument,	0,	'd'},
	{"foreground", 	required_argument,	0,	'f'},
#ifdef LOCAL_SUPPORT
	{"httpd", 	no_argument,		0,	'l'},
	{"httpd-port",	required_argument,	0,	'p'},
#endif /* LOCAL_SUPPORT */
	{"verbose",	required_argument,	0,	'v'},
	{"help",	no_argument,		0,	'h'},
	{"version",	no_argument,		0,	'V'},
	{} /* stop condition for iterator */
};

/**
 * Descriptions vor command line options
 */
const char *long_options_descs[] = {
	"configuration file",
	"log file",
	"run as periodically",
	"do not run in background",
#ifdef LOCAL_SUPPORT
	"activate local interface (tiny HTTPd which serves live readings)",
	"TCP port for HTTPd",
#endif /* LOCAL_SUPPORT */
	"enable verbose output",
	"show this help",
	"show version of vzlogger",
	NULL /* stop condition for iterator */
};

/**
 * Print available options and some other usefull information
 */
void usage(char *argv[]) {
	const char **desc = long_options_descs;
	const struct option *op = long_options;
	const meter_type_t *type = meter_types;

	printf("Usage: %s [options]\n\n", argv[0]);
	printf("  following options are available:\n");

	while (op->name && desc) {
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

	if (level <= options.verbosity) {
		gettimeofday(&now, NULL);
		timeinfo = localtime(&now.tv_sec);

		pos += sprintf(pos, "[");
		pos += strftime(pos, 16, "%b %d %H:%M:%S", timeinfo);

		if (id != NULL) {
			pos += sprintf(pos, "][%s]", (char *) id);
		}
		else {
			pos += sprintf(pos, "]");
		}

		while(pos - buffer < 24) {
			pos += sprintf(pos, " ");
		}

		va_start(args, id);
		pos += vsprintf(pos, format, args);
		va_end(args);

		fprintf((level > 0) ? stdout : stderr, "%s\n", buffer);

		if (options.logfd) {
			fprintf(options.logfd, "%s\n", buffer);
			fflush(options.logfd);
		}
	}
}

/* http://www.enderunix.org/docs/eng/daemon.php */
void daemonize() {
	if(getppid() == 1) {
		return; /* already a daemon */
	}

	int i = fork();
	if (i < 0) {
		exit(EXIT_FAILURE); /* fork error */
	}
	else if (i > 0) {
		exit(EXIT_SUCCESS); /* parent exits */
	}

	/* child (daemon) continues */

	setsid(); /* obtain a new process group */

	for (i=getdtablesize();i>=0;--i) {
		close(i); /* close all descriptors */
	}

	/* handle standart I/O */
	i = open("/dev/null", O_RDWR);
	dup(i);
	dup(i);

	chdir("/"); /* change working directory */
	umask(0022);

	/* ignore signals from parent tty */
	struct sigaction action;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	action.sa_handler = SIG_IGN;

	sigaction(SIGCHLD, &action, NULL); /* ignore child */
	sigaction(SIGTSTP, &action, NULL); /* ignore tty signals */
	sigaction(SIGTTOU, &action, NULL);
	sigaction(SIGTTIN, &action, NULL);
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
 * Parse options from command line
 */
void parse_options(int argc, char * argv[], options_t * options) {
	while (1) {
		int c = getopt_long(argc, argv, "c:o:p:lhVdfv:", long_options, NULL);

		/* detect the end of the options. */
		if (c == -1) break;

		switch (c) {
			case 'v':
				options->verbosity = atoi(optarg);
				break;

#ifdef LOCAL_SUPPORT
			case 'l':
				options->local = 1;
				break;

			case 'p': /* port for local interface */
				options->port = atoi(optarg);
				break;
#endif /* LOCAL_SUPPORT */

			case 'd':
				options->daemon = 1;
				break;

			case 'f':
				options->foreground = 1;
				break;

			case 'c': /* config file */
				options->config = strdup(optarg);
				break;

			case 'o': /* log file */
				options->log = strdup(optarg);
				break;

			case 'V':
				printf("%s\n", VERSION);
				exit(EXIT_SUCCESS);
				break;

			case '?':
			case 'h':
			default:
				usage(argv);
				exit((c == '?') ? EXIT_FAILURE : EXIT_SUCCESS);
		}
	}
}

/**
 * The application entrypoint
 */
int main(int argc, char *argv[]) {
	/* default options */
	options.config = "/etc/vzlogger.conf";
	options.log = "/var/log/vzlogger.log";
	options.logfd = NULL;
	options.port = 8080;
	options.verbosity = 0;
	options.comet_timeout = 30;
	options.buffer_length = 600;
	options.retry_pause = 15;
	options.daemon = FALSE;
	options.local = FALSE;
	options.logging = TRUE;

	/* bind signal handler */
	struct sigaction action;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	action.sa_handler = quit;

	sigaction(SIGINT, &action, NULL);	/* catch ctrl-c from terminal */
	sigaction(SIGHUP, &action, NULL);	/* catch hangup signal */
	sigaction(SIGTERM, &action, NULL);	/* catch kill signal */

	/* initialize adts and apis */
	curl_global_init(CURL_GLOBAL_ALL);
	list_init(&assocs);

	/* parse command line and file options */
	// TODO command line should have a higher priority as file
	parse_options(argc, argv, &options);
	parse_configuration(options.config, &assocs, &options);

	options.logging = (!options.local || options.daemon);

	if (!options.foreground && (options.daemon || options.local)) {
		print(1, "Daemonize process...", NULL);
		daemonize();
	}

	/* open logfile */
	if (options.log) {
		FILE *logfd = fopen(options.log, "a");

		if (logfd) {
			options.logfd = logfd;
			print(LOG_DEBUG, "Opened logfile %s", NULL, options.log);
		}
		else {
			print(LOG_ERROR, "Cannot open logfile %s: %s", NULL, options.log, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	if (assocs.size == 0) {
		print(6, "No meters found!", NULL);
		exit(EXIT_FAILURE);
	}

	/* open connection meters & start threads */
	foreach(assocs, it) {
		assoc_t *assoc = (assoc_t *) it->data;
		meter_t *mtr = &assoc->meter;

		int res = meter_open(mtr);
		if (res < 0) {
			print(LOG_ERROR, "Failed to open meter", mtr);
			exit(EXIT_FAILURE);
		}
		print(5, "Meter connected", mtr);
		pthread_create(&assoc->thread, NULL, &reading_thread, (void *) assoc);
		print(5, "Meter thread started", mtr);

		foreach(assoc->channels, it) {
			channel_t *ch = (channel_t *) it->data;

			/* set buffer length for perriodic meters */
			if (mtr->type->periodic && options.local) {
				ch->buffer.keep = ceil(options.buffer_length / (double) assoc->interval);
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
		print(5, "Starting local interface HTTPd on port %i", "http", options.port);
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
		print(8, "Stopping local interface HTTPd", "http");
		MHD_stop_daemon(httpd_handle);
	}
#endif /* LOCAL_SUPPORT */

	/* householding */
	list_free(&assocs);
	curl_global_cleanup();
	if (options.logfd) {
		fclose(options.logfd);
	}

	return EXIT_SUCCESS;
}
