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

#include <ctype.h>
#include <errno.h>
#include <regex>
#include <stdio.h>

#include "Channel.hpp"
#include "config.hpp"
#include <Config_Options.hpp>
#include <VZException.hpp>
#ifdef ENABLE_MQTT
#include "mqtt.hpp"
#endif

static const char *option_type_str[] = {"null",   "boolean", "double", "int",
										"object", "array",   "string"};

Config_Options::Config_Options()
	: _config("/etc/vzlogger.conf"), _log(""), _pds(0), _port(8080), _verbosity(0),
	  _comet_timeout(30), _buffer_length(-1), _retry_pause(15), _local(false), _foreground(false),
	  _time_machine(false) {
	_logfd = NULL;
}

Config_Options::Config_Options(const std::string filename)
	: _config(filename), _log(""), _pds(0), _port(8080), _verbosity(0), _comet_timeout(30),
	  _buffer_length(-1), _retry_pause(15), _local(false), _foreground(false),
	  _time_machine(false) {
	_logfd = NULL;
}

void Config_Options::config_parse(MapContainer &mappings) {
	struct json_object *json_cfg = NULL;
	struct json_tokener *json_tok = json_tokener_new();

	char buf[JSON_FILE_BUF_SIZE];
	int line = 0;

	/* open configuration file */
	FILE *file = fopen(_config.c_str(), "r");
	if (file == NULL) {
		print(log_alert, "Cannot open configfile %s: %s", NULL, _config.c_str(),
			  strerror(errno)); /* why didn't the file open? */
		throw vz::VZException("Cannot open configfile.");
	} else {
		print(log_info, "Start parsing configuration from %s", NULL, _config.c_str());
	}

#ifdef HAVE_CPP_REGEX
	std::regex regex("^\\s*(//(.*|)|$)"); // if you change this pls adjust unit test in
										  // ut_api_volkszaehler.cpp regex_for_configs as well!
#endif

	/* parse JSON */
	while (fgets(buf, JSON_FILE_BUF_SIZE, file)) {
		line++;

		if (json_cfg != NULL) {
#ifdef HAVE_CPP_REGEX
			// let's ignore whitespace and single line comments here:
			if (!std::regex_match((const char *)buf, regex)) {
#else
			// let's accept at least whitespace here:
			std::string strline(buf);
			if (!std::all_of(strline.begin(), strline.end(), isspace)) {
#endif
				print(log_alert, "extra data after end of configuration in %s:%d", NULL,
					  _config.c_str(), line);
				throw vz::VZException("extra data after end of configuration");
			}
		} else {

			json_cfg = json_tokener_parse_ex(json_tok, buf, strlen(buf));

			if (json_tok->err > 1) {
				print(log_alert, "Error in %s:%d %s at offset %d", NULL, _config.c_str(), line,
					  json_tokener_error_desc(json_tok->err), json_tok->char_offset);
				json_object_put(json_cfg);
				json_cfg = 0;
				throw vz::VZException("Parse configuaration failed.");
			}
		}
	}

	/* householding */
	fclose(file);
	json_tokener_free(json_tok);

	if (json_cfg == NULL)
		throw vz::VZException("configuration file incomplete, missing closing braces/parens?");

	try {
		/* parse options */
		json_object_object_foreach(json_cfg, key, value) {
			enum json_type type = json_object_get_type(value);

			if (strcmp(key, "daemon") == 0 && type == json_type_boolean) {
				if (!json_object_get_boolean(value)) {
					throw vz::VZException("\"daemon\" option is not supported anymore, "
										  "you probably want to use -f instead.");
				}
			} else if (strcmp(key, "log") == 0 && type == json_type_string) {
				_log = json_object_get_string(value);
			} else if (strcmp(key, "retry") == 0 && type == json_type_int) {
				_retry_pause = json_object_get_int(value);
			} else if (strcmp(key, "verbosity") == 0 && type == json_type_int) {
				_verbosity = json_object_get_int(value);
			} else if (strcmp(key, "local") == 0) {
				json_object_object_foreach(value, key, local_value) {
					enum json_type local_type = json_object_get_type(local_value);

					if (strcmp(key, "enabled") == 0 && local_type == json_type_boolean) {
						_local = json_object_get_boolean(local_value);
					} else if (strcmp(key, "port") == 0 && local_type == json_type_int) {
						_port = json_object_get_int(local_value);
					} else if (strcmp(key, "timeout") == 0 && local_type == json_type_int) {
						_comet_timeout = json_object_get_int(local_value);
					} else if (strcmp(key, "buffer") == 0 && local_type == json_type_int) {
						_buffer_length = json_object_get_int(local_value);
						if (!_buffer_length)
							_buffer_length =
								-1; // 0 makes no sense, use size based mode with 1 element
					} else if (strcmp(key, "index") == 0 && local_type == json_type_boolean) {
						_channel_index = json_object_get_boolean(local_value);
					} else {
						print(log_alert, "Ignoring invalid field or type: %s=%s (%s)", NULL, key,
							  json_object_get_string(local_value), option_type_str[local_type]);
					}
				}
			} else if ((strcmp(key, "sensors") == 0 || strcmp(key, "meters") == 0) &&
					   type == json_type_array) {
				int len = json_object_array_length(value);
				for (int i = 0; i < len; i++) {
					Json::Ptr jso(new Json(json_object_array_get_idx(value, i)));
					config_parse_meter(mappings, jso);
				}
			} else if ((strcmp(key, "push") == 0) && type == json_type_array) {
				int len = json_object_array_length(value);
				if (!_pds && len > 0) {
					_pds = new PushDataServer(value);
				} else
					print(log_error, "Ignoring push entry due to empty array or duplicate section",
						  "push");
			}
#ifdef ENABLE_MQTT
			else if ((strcmp(key, "mqtt") == 0) && type == json_type_object) {
				if (!mqttClient) {
					mqttClient = new MqttClient(value);
					if (!mqttClient->isConfigured()) {
						delete mqttClient;
						mqttClient = 0;
						print(log_debug, "mqtt client not configured. stopped.", "mqtt");
					}
				} else
					print(log_error, "Ignoring mqtt entry due to empty array or duplicate section",
						  "mqtt");
			}
#endif
			else if ((strcmp(key, "i_have_a_time_machine") == 0) && type == json_type_boolean) {
				_time_machine = json_object_get_boolean(value);
			} else {
				print(log_alert, "Ignoring invalid field or type: %s=%s (%s)", NULL, key,
					  json_object_get_string(value), option_type_str[type]);
			}
		}
	} catch (std::exception &e) {
		json_object_put(json_cfg); /* free allocated memory */
		std::stringstream oss;
		oss << e.what();
		print(log_alert, "parse configuration failed due to:", "", oss.str().c_str());
		throw;
	}

	print(log_debug, "Have %d meters.", NULL, mappings.size());
	json_object_put(json_cfg); /* free allocated memory */
}

void Config_Options::config_parse_meter(MapContainer &mappings, Json::Ptr jso) {
	std::list<Json> json_channels;
	std::list<Option> options;

	json_object_object_foreach(jso->Object(), key, value) {
		enum json_type type = json_object_get_type(value);

		if (strcmp(key, "channels") == 0 && type == json_type_array) {
			int len = json_object_array_length(value);
			for (int i = 0; i < len; i++) {
				json_channels.push_back(Json(json_object_array_get_idx(value, i)));
			}
		} else if (strcmp(key, "channel") == 0 && type == json_type_object) {
			json_channels.push_back(Json(value));
		} else { /* all other options will be passed to meter_init() */
			Option option(key, value);
			options.push_back(option);
		}
	}

	/* init meter */
	MeterMap metermap(options);

	print(log_info, "New meter initialized (protocol=%s)", NULL /*(mapping*/,
		  meter_get_details(metermap.meter()->protocolId())->name);

	/* init channels */
	for (std::list<Json>::iterator it = json_channels.begin(); it != json_channels.end(); it++) {
		config_parse_channel(*it, metermap);
	}

	/* householding */
	mappings.push_back(metermap);
}

void Config_Options::config_parse_channel(Json &jso, MeterMap &mapping) {
	std::list<Option> options;
	const char *uuid = NULL;
	const char *id_str = NULL;
	std::string apiProtocol_str;

	print(log_debug, "Configure channel.", NULL);
	json_object_object_foreach(jso.Object(), key, value) {
		enum json_type type = json_object_get_type(value);

		if (strcmp(key, "uuid") == 0 && type == json_type_string) {
			uuid = json_object_get_string(value);
		}
		// else if (strcmp(key, "middleware") == 0 && type == json_type_string) {
		//	middleware = json_object_get_string(value);
		//}
		else if (strcmp(key, "identifier") == 0 && type == json_type_string) {
			id_str = json_object_get_string(value);
		} else if (strcmp(key, "api") == 0 && type == json_type_string) {
			apiProtocol_str = json_object_get_string(value);
		} else { /* all other options will be passed to meter_init() */
			Option option(key, value);
			options.push_back(option);
			// print(log_alert, "Ignoring invalid field or type: %s=%s (%s)",
			//	NULL, key, json_object_get_string(value), option_type_str[type]);
		}
	}

	/* check uuid and middleware */
	if (uuid == NULL) {
		print(log_alert, "Missing UUID", NULL);
		throw vz::VZException("Missing UUID");
	}
	if (!config_validate_uuid(uuid)) {
		print(log_alert, "Invalid UUID: %s", NULL, uuid);
		throw vz::VZException("Invalid UUID.");
	}
	// check if identifier is set. If not, use default
	if (id_str == NULL) {
		print(log_error, "Identifier is not set. Using default value 'NilIdentifier'.", NULL);
		id_str = "NilIdentifier";
	}
	// if (middleware == NULL) {
	//	print(log_error, "Missing middleware", NULL);
	//	throw vz::VZException("Missing middleware.");
	//}
	if (apiProtocol_str.length() == 0) {
		apiProtocol_str = "volkszaehler";
	}

	/* parse identifier */
	ReadingIdentifier::Ptr id;
	try {
		id = reading_id_parse(mapping.meter()->protocolId(), (const char *)id_str);
	} catch (vz::VZException &e) {
		std::stringstream oss;
		oss << e.what();
		print(log_alert, "Invalid id: %s due to: '%s'", NULL, id_str, oss.str().c_str());
		throw vz::VZException("Invalid reader.");
	}

	Channel::Ptr ch(new Channel(options, apiProtocol_str.c_str(), uuid, id));
	print(log_info, "New channel initialized (uuid=...%s api=%s id=%s)", ch->name(), uuid + 30,
		  apiProtocol_str.c_str(), (id_str) ? id_str : "(none)");
	mapping.push_back(ch);
}

bool config_validate_uuid(const char *uuid) {
	for (const char *p = uuid; *p; p++) {
		switch (p - uuid) {
		case 8:
		case 13:
		case 18:
		case 23:
			if (*p != '-')
				return false;
			else
				break;

		default:
			if (!isxdigit(*p))
				return false;
			else
				break;
		}
	}
	return true;
}
