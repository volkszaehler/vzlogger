/**
 * Parsing Apache HTTPd-like configuration
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
#include <errno.h>

#include "configuration.h"
#include "channel.h"

extern const meter_type_t meter_types[];

void parse_configuration(char *filename, list_t *assocs, options_t *options) {
	struct json_object *json_config = NULL;
	struct json_tokener *json_tok = json_tokener_new();

	char buf[JSON_FILE_BUF_SIZE];
	int line = 0;

	/* open configuration file */
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		print(LOG_ERROR, "Cannot open configfile %s: %s", NULL, filename, strerror(errno)); /* why didn't the file open? */
		exit(EXIT_FAILURE);
	}
	else {
		print(2, "Start parsing configuration from %s", NULL, filename);
	}

	/* parse JSON */
	while(fgets(buf, JSON_FILE_BUF_SIZE, file)) {
		line++;

		json_config = json_tokener_parse_ex(json_tok, buf, strlen(buf));

		if (json_tok->err > 1) {
			print(-1, "Error in %s:%d %s at offset %d", NULL, filename, line, json_tokener_errors[json_tok->err], json_tok->char_offset);
			exit(EXIT_FAILURE);
		}
	}

	fclose(file);
	json_tokener_free(json_tok);

	/* read settings */
	json_object_object_foreach(json_config, key, value) {
		if (strcmp(key, "daemon") == 0 && check_type(key, value, json_type_boolean)) {
			options->daemon = json_object_get_boolean(value);
		}
		else if (strcmp(key, "foreground") == 0 && check_type(key, value, json_type_boolean)) {
			options->foreground = json_object_get_boolean(value);
		}
		else if (strcmp(key, "log") == 0 && check_type(key, value, json_type_string)) {
			options->log = strdup(json_object_get_string(value));
		}
		else if (strcmp(key, "retry") == 0 && check_type(key, value, json_type_int)) {
			options->retry_pause = json_object_get_int(value);
		}
		else if (strcmp(key, "verbosity") == 0 && check_type(key, value, json_type_int)) {
			options->verbosity = json_object_get_int(value);
		}
		else if (strcmp(key, "local") == 0) {
			json_object_object_foreach(value, key, local_value) {
				if (strcmp(key, "enabled") == 0 && check_type(key, local_value, json_type_boolean)) {
					options->local = json_object_get_boolean(local_value);
				}
				else if (strcmp(key, "port") == 0 && check_type(key, local_value, json_type_int)) {
					options->port = json_object_get_int(local_value);
				}
				else if (strcmp(key, "timeout") == 0 && check_type(key, local_value, json_type_int)) {
					options->comet_timeout = json_object_get_int(local_value);
				}
				else if (strcmp(key, "buffer") == 0 && check_type(key, local_value, json_type_int)) {
					options->buffer_length = json_object_get_int(local_value);
				}
				else if (strcmp(key, "index") == 0 && check_type(key, local_value, json_type_boolean)) {
					options->channel_index = json_object_get_boolean(local_value);
				}
				else {
					print(-1, "Invalid field: %s", NULL, key);
					exit(EXIT_FAILURE);
				}
			}
		}
		else if ((strcmp(key, "sensors") == 0 || strcmp(key, "meters") == 0) && check_type(key, value, json_type_array)) {
			int len = json_object_array_length(value);
			for (int i = 0; i < len; i++) {
				assoc_t *as = parse_meter(json_object_array_get_idx(value, i));

				if (as != NULL) {
					list_push(assocs, as);
				}
			}
		}
		else {
			print(-1, "Invalid field: %s", NULL, key);
			exit(EXIT_FAILURE);
		}
	}

	json_object_put(json_config);
}

assoc_t * parse_meter(struct json_object *jso) {
	list_t json_channels;

	const meter_type_t *type = NULL;
	const char *connection = NULL;
	const char *protocol = NULL;
	int enabled = TRUE;
	int interval = 0;

	list_init(&json_channels);

	json_object_object_foreach(jso, key, value) {
		if (strcmp(key, "enabled") == 0 && check_type(key, value, json_type_boolean)) {
			enabled = json_object_get_boolean(value);
		}
		else if (strcmp(key, "protocol") == 0 && check_type(key, value, json_type_string)) {
			protocol = json_object_get_string(value);

			for (type = meter_types; type->name != NULL; type++) { /* linear search */
				if (strcmp(type->name, protocol) == 0) break;
			}

			if (type == NULL) {
				print(-1, "Invalid protocol: %s", NULL, protocol);
			}
		}
		else if (strcmp(key, "connection") == 0 && check_type(key, value, json_type_string)) {
			connection = json_object_get_string(value);
		}
		else if (strcmp(key, "interval") == 0 && check_type(key, value, json_type_int)) {
			interval = json_object_get_int(value);
		}
		else if (strcmp(key, "channels") == 0 && check_type(key, value, json_type_array)) {
			int len = json_object_array_length(value);
			for (int i = 0; i < len; i++) {
				list_push(&json_channels, json_object_array_get_idx(value, i));
			}
		}
		else if (strcmp(key, "channel") == 0 && check_type(key, value, json_type_object)) {
			list_push(&json_channels, value);
		}
		else {
			print(-1, "Invalid field: %s", NULL, key);
			exit(EXIT_FAILURE);
		}
	}

	if (type == NULL) {
		print(-1, "Missing protocol", NULL);
		exit(EXIT_FAILURE);
	}
	else if (connection == NULL) {
		print(-1, "Missing connection", NULL);
		exit(EXIT_FAILURE);
	}
	else if (enabled == TRUE) {
		/* init meter */
		assoc_t *assoc = malloc(sizeof(assoc_t));
		assoc->interval = interval;

		list_init(&assoc->channels);
		meter_init(&assoc->meter, type, connection);
		print(5, "New meter initialized (protocol=%s, connection=%s, interval=%d)", assoc, protocol, connection, assoc->interval);

		/* init channels */
		struct json_object *jso;
		while ((jso = list_pop(&json_channels)) != NULL) {
			channel_t *ch = parse_channel(jso);

			if (ch != NULL) {
				list_push(&assoc->channels, ch);
			}
		}

		return assoc;
	}
	else {
		return NULL;
	}
}

channel_t * parse_channel(struct json_object *jso) {
	const char *uuid = NULL;
	const char *middleware = NULL;
	const char *identifier = NULL;
	int enabled = TRUE;

	json_object_object_foreach(jso, key, value) {
		if (strcmp(key, "uuid") == 0 && check_type(key, value, json_type_string)) {
			uuid = json_object_get_string(value);
		}
		else if (strcmp(key, "middleware") == 0 && check_type(key, value, json_type_string)) {
			middleware = json_object_get_string(value);
		}
		else if (strcmp(key, "identifier") == 0 && check_type(key, value, json_type_string)) {
			identifier = json_object_get_string(value);
		}
		else if (strcmp(key, "enabled") == 0 && check_type(key, value, json_type_boolean)) {
			enabled = json_object_get_boolean(value);
		}
		else {
			print(-1, "Invalid field: %s", NULL, key);
			exit(EXIT_FAILURE);
		}
	}

	if (uuid == NULL) {
		print(-1, "Missing UUID", NULL);
		exit(EXIT_FAILURE);
	}
	else if (middleware == NULL) {
		print(-1, "Missing middleware", NULL);
		exit(EXIT_FAILURE);
	}
	else if (enabled == TRUE) {
		// TODO other identifiers are not supported at the moment
		reading_id_t id;
		// TODO: at present (2011-11-05) aliases for identifiers don't work because "lookup aliases" is not (re-)implemented yet in src/obis.c; for now, only the obis identifiers (like "1.8.0") are allowed, so
		if (obis_parse(&id.obis, identifier, strlen(identifier)) != 0) {
			print(-1, "Invalid identifier: %s", NULL, identifier);
			exit(EXIT_FAILURE);
		}

		char obis_str[6*3+5+1];
		obis_unparse(id.obis, obis_str, 6*3+5+1);

		channel_t *ch = malloc(sizeof(channel_t));
		channel_init(ch, uuid, middleware, id);
		print(5, "New channel initialized (uuid=...%s middleware=%s obis=%s (%s))", ch, uuid+30, middleware, obis_str, identifier);

		return ch;
	}
	else {
		return NULL;
	}
}

int check_type(char *key, struct json_object *jso, enum json_type type) {
	char *json_types[] = { "null", "boolean", "double", "int", "object", "array", "string" };

	if (json_object_get_type(jso) != type) {
		print(-1, "Invalid variable type for field %s: %s (%s)", NULL, key, json_object_get_string(jso), json_types[json_object_get_type(jso)]);
		exit(EXIT_FAILURE);
	}
	else {
		return 1;
	}
}

