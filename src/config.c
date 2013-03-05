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
#include <ctype.h>

#include "config.h"
#include "channel.h"

static const char *option_type_str[] = { "null", "boolean", "double", "int", "object", "array", "string" };

int config_parse(const char *filename, list_t *mappings, config_options_t *options) {
	struct json_object *json_cfg = NULL;
	struct json_tokener *json_tok = json_tokener_new();

	char buf[JSON_FILE_BUF_SIZE];
	int line = 0;

	/* open configuration file */
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		print(log_error, "Cannot open configfile %s: %s", NULL, filename, strerror(errno)); /* why didn't the file open? */
		exit(EXIT_FAILURE);
	}
	else {
		print(log_info, "Start parsing configuration from %s", NULL, filename);
	}

	/* parse JSON */
	while(fgets(buf, JSON_FILE_BUF_SIZE, file)) {
		line++;

		if (json_cfg){
			print(log_error, "extra data after end of configuration in %s:%d", NULL, filename, line);
			exit(EXIT_FAILURE);
		}

		json_cfg = json_tokener_parse_ex(json_tok, buf, strlen(buf));

		if (json_tok->err > 1) {
			print(log_error, "Error in %s:%d %s at offset %d", NULL, filename, line, json_tokener_errors[json_tok->err], json_tok->char_offset);
			exit(EXIT_FAILURE);
		}
	}

	/* householding */
	fclose(file);
	json_tokener_free(json_tok);

	if (!json_cfg){
		print(log_error, "configuration file incomplete, missing closing braces/parens?",NULL);
		exit(EXIT_FAILURE);
	}

	/* parse options */
	json_object_object_foreach(json_cfg, key, value) {
		enum json_type type = json_object_get_type(value);

		if (strcmp(key, "daemon") == 0 && type == json_type_boolean) {
			options->daemon = json_object_get_boolean(value);
		}
		else if (strcmp(key, "foreground") == 0 && type == json_type_boolean) {
			options->foreground = json_object_get_boolean(value);
		}
		else if (strcmp(key, "log") == 0 && type == json_type_string) {
			options->log = strdup(json_object_get_string(value));
		}
		else if (strcmp(key, "retry") == 0 && type == json_type_int) {
			options->retry_pause = json_object_get_int(value);
		}
		else if (strcmp(key, "verbosity") == 0 && type == json_type_int) {
			options->verbosity = json_object_get_int(value);
		}
		else if (strcmp(key, "local") == 0) {
			json_object_object_foreach(value, key, local_value) {
				enum json_type local_type = json_object_get_type(local_value);

				if (strcmp(key, "enabled") == 0 && local_type == json_type_boolean) {
					options->local = json_object_get_boolean(local_value);
				}
				else if (strcmp(key, "port") == 0 && local_type == json_type_int) {
					options->port = json_object_get_int(local_value);
				}
				else if (strcmp(key, "timeout") == 0 && local_type == json_type_int) {
					options->comet_timeout = json_object_get_int(local_value);
				}
				else if (strcmp(key, "buffer") == 0 && local_type == json_type_int) {
					options->buffer_length = json_object_get_int(local_value);
				}
				else if (strcmp(key, "index") == 0 && local_type == json_type_boolean) {
					options->channel_index = json_object_get_boolean(local_value);
				}
				else {
					print(log_error, "Ignoring invalid field or type: %s=%s (%s)",
						NULL, key, json_object_get_string(local_value), option_type_str[local_type]);
				}
			}
		}
		else if ((strcmp(key, "sensors") == 0 || strcmp(key, "meters") == 0) && type == json_type_array) {
			int len = json_object_array_length(value);
			for (int i = 0; i < len; i++) {
				map_t *mapping = config_parse_meter(json_object_array_get_idx(value, i));
				if (mapping == NULL) {
					return ERR;
				}

				list_push(mappings, mapping);
			}
		}
		else {
			print(log_error, "Ignoring invalid field or type: %s=%s (%s)",
				NULL, key, json_object_get_string(value), option_type_str[type]);
		}
	}

	json_object_put(json_cfg); /* free allocated memory */

	return SUCCESS;
}

map_t * config_parse_meter(struct json_object *jso) {
	list_t json_channels;
	list_t options;
	list_init(&json_channels);
	list_init(&options);

	json_object_object_foreach(jso, key, value) {
		enum json_type type = json_object_get_type(value);

		if (strcmp(key, "channels") == 0 && type == json_type_array) {
			int len = json_object_array_length(value);
			for (int i = 0; i < len; i++) {
				list_push(&json_channels, json_object_array_get_idx(value, i));
			}
		}
		else if (strcmp(key, "channel") == 0 && type == json_type_object) {
			list_push(&json_channels, value);
		}
		else { /* all other options will be passed to meter_init() */
			option_t *option = malloc(sizeof(option_t));

			if (option_init(value, key, option) != SUCCESS) {
				print(log_error, "Ignoring invalid type: %s=%s (%s)",
					NULL, key, json_object_get_string(value), option_type_str[type]);
			}

			list_push(&options, option);
		}
	}

	/* init meter */
	map_t *mapping = malloc(sizeof(map_t));
	list_init(&mapping->channels);

	if (meter_init(&mapping->meter, options) != SUCCESS) {
		print(log_error, "Failed to initialize meter. Aborting.", mapping);
		free(mapping);
		return NULL;
	}

	print(log_info, "New meter initialized (protocol=%s)", mapping, meter_get_details(mapping->meter.protocol)->name);

	/* init channels */
	struct json_object *json_channel;
	while ((json_channel = list_pop(&json_channels)) != NULL) {
		channel_t *ch = config_parse_channel(json_channel, mapping->meter.protocol);
		if (ch == NULL) {
			free(mapping);
			return NULL;
		}

		list_push(&mapping->channels, ch);
	}

	/* householding */
	list_free(&options);

	return mapping;
}

channel_t * config_parse_channel(struct json_object *jso, meter_protocol_t protocol) {
	const char *uuid = NULL;
	const char *middleware = NULL;
	const char *id_str = NULL;

	json_object_object_foreach(jso, key, value) {
		enum json_type type = json_object_get_type(value);

		if (strcmp(key, "uuid") == 0 && type == json_type_string) {
			uuid = json_object_get_string(value);
		}
		else if (strcmp(key, "middleware") == 0 && type == json_type_string) {
			middleware = json_object_get_string(value);
		}
		else if (strcmp(key, "identifier") == 0 && type == json_type_string) {
			id_str = json_object_get_string(value);
		}
		else {
			print(log_error, "Ignoring invalid field or type: %s=%s (%s)",
				NULL, key, json_object_get_string(value), option_type_str[type]);
		}
	}

	/* check uuid and middleware */
	if (uuid == NULL) {
		print(log_error, "Missing UUID", NULL);
		return NULL;
	}
	else if (!config_validate_uuid(uuid)) {
		print(log_error, "Invalid UUID: %s", NULL, uuid);
		return NULL;
	}
	else if (middleware == NULL) {
		print(log_error, "Missing middleware", NULL);
		return NULL;
	}

	/* parse identifier */
	reading_id_t id;
	if (id_str != NULL && reading_id_parse(protocol, &id, id_str) != SUCCESS) {
		print(log_error, "Invalid id: %s", NULL, id_str);
		return NULL; /* error occured */
	}

	channel_t *ch = malloc(sizeof(channel_t));
	channel_init(ch, uuid, middleware, id);
	print(log_info, "New channel initialized (uuid=...%s middleware=%s id=%s)", ch, uuid+30, middleware, (id_str) ? id_str : "(none)");

	return ch;
}

int config_validate_uuid(const char *uuid) {
	for (const char *p = uuid; *p; p++) {
		switch (p - uuid) {
			case 8:
			case 13:
			case 18:
			case 23:
				if (*p != '-') return FALSE;
				else break;

			default:
				if (!isxdigit(*p)) return FALSE;
				else break;
		}
	}
	return TRUE;
}

int option_init(struct json_object *jso, char *key,  option_t *option) {
	option_value_t val;

	switch (json_object_get_type(jso)) {
		case json_type_string:	val.string = json_object_get_string(jso);   break;
		case json_type_int:	val.integer = json_object_get_int(jso);     break;
		case json_type_boolean:	val.boolean = json_object_get_boolean(jso); break;
		case json_type_double:	val.floating = json_object_get_double(jso); break;
		default:		return ERR;
	}

	option->key = key;
	option->type = json_object_get_type(jso);
	option->value = val;

	return SUCCESS;
}

