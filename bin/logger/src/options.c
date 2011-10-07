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

#include <meter.h>

#include "options.h"
#include "channel.h"
#include "vzlogger.h"

extern meter_type_t meter_types[];

options_t options = { /* setting default options */
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
 * Options for config file
 */
struct {
	char *key;
	int level;
}
 

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
		int c = getopt_long(argc, argv, "i:c:p:lhVdv::", long_options, NULL);

		/* detect the end of the options. */
		if (c == -1) break;

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

			case '?':
			case 'h':
			default:
				usage(argv);
				exit((c == '?') ? EXIT_FAILURE : EXIT_SUCCESS);
		}
	}
	
	opts->logging = (!opts->local || opts->daemon);
}

void parse_channels(const char *filename, list_t *assocs) {
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
	
	/* compile regular expressions */
	regex_t re_uuid, re_middleware, re_section, re_value;
	regcomp(&re_uuid, "^[:xdigit:]{8}-([:xdigit:]{4}-){3}[:xdigit:]{12}$", REG_EXTENDED | REG_NOSUB);
	regcomp(&re_middleware, "^https?://[[:alnum:].-]+(\\.[:alpha:]{2,6})?(:[:digit:]{1,5})?(/\\S*)?$", REG_EXTENDED | REG_NOSUB);

	regcomp(&re_section, "^[:blank:]*<(/?)([:alpha]+)>[:blank:]*$", REG_EXTENDED);
	regcomp(&re_value, "^[:blank:]*([:alpha]+)[:blank:]+(.+)[:blank:]*$", REG_EXTENDED);
	
	struct options {
		char *key;
		enum context;
		//char regex;
	}
	
	/* read a line */
	while ((line = fgets(buffer, 256, file)) != NULL) {
		if (regex(&re_section) == 0) {
			
		}
		else if (regex(&re_value) == 0) {
			switch (context) {
				case GLOBAL:
					/* no values allowed here */
					break;
					
				case METER:
					if (strcasecmp(key, "protocol") == 0) {
				
					}
					else if (strcasecmp(key, "interval") == 0) {
				
					}
					else if (strcasecmp(key, "host") == 0) {
				
					}
					else if (strcasecmp(key, "port") == 0) {
				
					}
					break;
				
				case CHANNEL:
					if (strcasecmp(key, "uuid") == 0) {
				
					}
					else if (strcasecmp(key, "middlware") == 0) {
				
					}
					else if (strcasecmp(key, "identifier") == 0) {
				
					}
				
					break;
				
				char *key, *value;
				
				
			}
		}
	
	}
	
	fclose(file);
	free(buffer);
	regfree(&re_middleware);
	regfree(&re_uuid);
}


