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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <regex.h>

#include "../../config.h"

#include "list.h"
#include "options.h"
#include "channel.h"
#include "vzlogger.h"

extern meter_type_t meter_types[];

const options_t opts = { /* setting default options */
	"/etc/vzlogger.conf",	/* config file */
	8080,			/* port for local interface */
	0,			/* verbosity level */
	0,			/* daemon mode */
	0			/* local interface */
};

/**
 * Command line options
 */
const struct option long_options[] = {
	{"config", 	required_argument,	0,	'c'},
	{"daemon", 	required_argument,	0,	'd'},
#ifdef LOCAL_SUPPORT
	{"local", 	no_argument,		0,	'l'},
	{"local-port",	required_argument,	0,	'p'},
#endif /* LOCAL_SUPPORT */
	{"verbose",	optional_argument,	0,	'v'},
	{"help",	no_argument,		0,	'h'},
	{"version",	no_argument,		0,	'V'},
	{} /* stop condition for iterator */
};

/**
 * Descriptions vor command line options
 */
char *long_options_descs[] = {
	"config file with channel -> uuid mapping",
	"run as daemon",
#ifdef LOCAL_SUPPORT
	"activate local interface (tiny webserver)",
	"TCP port for local interface",
#endif /* LOCAL_SUPPORT */
	"enable verbose output",
	"show this help",
	"show version of vzlogger",
	"" /* stop condition for iterator */
};

/**
 * Parse options from command line
 */
void parse_options(int argc, char * argv[], options_t * opts) {
	while (1) {
		/* getopt_long stores the option index here. */
		int option_index = 0;

		int c = getopt_long(argc, argv, "i:c:p:lhVdv::", long_options, &option_index);

		/* detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {
			case 'v':
				opts->verbose = (optarg == NULL) ? 1 : atoi(optarg);
				break;

#ifdef LOCAL_SUPPORT
			case 'l':
				opts->local = 1;
				break;
				
			case 'p': /* port for local interface */
				opts->port = atoi(optarg);
				break;
#endif /* LOCAL_SUPPORT */

			case 'd':
				opts->daemon = 1;
				break;

			case 'c': /* read config file */
				opts->config = (char *) malloc(strlen(optarg)+1);
				strcpy(opts->config, optarg);
				break;

			case 'V':
				printf("%s\n", VERSION);
				exit(EXIT_SUCCESS);
				break;

			case 'h':
			case '?':
				usage(argv);
				exit((c == '?') ? EXIT_FAILURE : EXIT_SUCCESS);
		}
	}
}

void parse_channels(char *filename, list_t *chans) {
	FILE *file = fopen(filename, "r"); /* open configuration */
	
	if (!filename) { /* nothing found */
		print(-1, "No config file found! Please specify with --config!\n", NULL);
		exit(EXIT_FAILURE);
	}

	if (file == NULL) {
		perror(filename); /* why didn't the file open? */
		exit(EXIT_FAILURE);
	}
	else {
		print(2, "Start parsing configuration from %s", NULL, filename);
	}

	int lineno = 1;
	char *buffer = malloc(256);
	char *tokens[5];
	char *line;
	
	/* compile regular expressions */
	regex_t re_uuid, re_middleware;
	regcomp(&re_uuid, "^[a-f0-9]{8}-([a-f0-9]{4}-){3,3}[a-f0-9]{12}$", REG_EXTENDED | REG_ICASE | REG_NOSUB);
	regcomp(&re_middleware, "^https?://[a-z0-9.-]+\\.[a-z]{2,6}(/\\S*)?$", REG_EXTENDED | REG_ICASE | REG_NOSUB);

	/*regerror(err, &re_uuid, buffer, 256);
        printf("Error analyzing regular expression: %s.\n", buffer);*/

	while ((line = fgets(buffer, 256, file)) != NULL) {	/* read a line */
		line[strcspn(line, "\n\r;")] = '\0';		/* strip newline and comments */
		if (strlen(line) == 0) continue;		/* skip empty lines */

		/* channel properties */
		char *middleware, *options, *uuid;
		unsigned long interval;
		channel_t ch;
		
		meter_type_t *type;
		meter_conn_t connection;

		/* parse tokens (required) */
		memset(tokens, 0, 5);
		for (int i = 0; i < 5; i++) {
			do {
				tokens[i] = strsep(&line, " \t");
			} while (tokens[i] == NULL && line != NULL);
		}
		
		/* protocol (required) */
		for (type = meter_types; type->name != NULL; type++) { /* linear search */
			if (strcmp(type->name, tokens[0]) == 0) break;
		}
		
					
		if (type->name == NULL) { /* reached end */
			print(-1, "Invalid protocol: %s in %s:%i", NULL, tokens[0], filename, lineno);
			exit(EXIT_FAILURE);
		}
		
		/* middleware (required) */
		middleware = tokens[1];
		if (regexec(&re_middleware, middleware, 0, NULL, 0) == 0) {
			print(-1, "Invalid interval: %s in %s:%i", NULL, tokens[1], filename, lineno);
			exit(EXIT_FAILURE);
		}

		/* uuid (required) */
		uuid = tokens[2];
		if (regexec(&re_uuid, uuid, 0, NULL, 0) != 0) {
			print(-1, "Invalid uuid: %s in %s:%i", NULL, tokens[2], filename, lineno);
			exit(EXIT_FAILURE);
		}
		
		/* interval (only if protocol is sensor) */
		if (type->periodical) {
			interval = strtol(tokens[3], (char **) NULL, 10);
			if (errno == EINVAL || errno == ERANGE) {
				print(-1, "Invalid interval: %s in %s:%i", NULL, tokens[3], filename, lineno);
				exit(EXIT_FAILURE);
			}
		}
		else {
			interval = 0;
		}
		
		/* connection */
		
		/* options (optional) */
		options = tokens[type->periodical ? 4 : 3];

		channel_init(&ch, uuid, middleware, interval, options, type);
		print(1, "Parsed (protocol=%s interval=%i uuid=%s middleware=%s options=%s)", &ch, type->name, interval, uuid, middleware, options);

		list_push(chans, ch);
		lineno++;
	}
	
	fclose(file);
	free(buffer);
	regfree(&re_middleware);
	regfree(&re_uuid);
}
