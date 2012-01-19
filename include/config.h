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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <json/json.h>

#include "meter.h"
#include "list.h"
#include "options.h"

/**
 * General options from CLI
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
} config_options_t;

/* forward declarartions */
struct map;
struct channel;

/**
 * Reads key, value and type from JSON object
 *
 * @param jso the JSON object
 * @param key the key of the option
 * @param option a pointer to a uninitialized option_t structure
 * @return 0 on succes, <0 on error
 */
int option_init(struct json_object *jso, char *key,  option_t *option);

/**
 * Validate UUID
 * Match against something like: '550e8400-e29b-11d4-a716-446655440000'
 *
 * @param const char *uuid a string containing to uuid
 * @return int non-zero on success
 */
int config_validate_uuid(const char *uuid);

/**
 * Parse JSON formatted configuration file
 *
 * @param const char *filename the path of the configuration file
 * @param list_t *mappings a pointer to a list, where new channel<->meter mappings should be stored
 * @param config_options_t *options a pointer to a structure of global configuration options
 * @return int non-zero on success
 */
int config_parse(const char *filename, list_t *mappings, config_options_t *options);

struct channel * config_parse_channel(struct json_object *jso, meter_protocol_t protocol);
struct map * config_parse_meter(struct json_object *jso);

#endif /* _CONFIG_H_ */
