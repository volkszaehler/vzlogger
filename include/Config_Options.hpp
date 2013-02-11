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

#include <list>
#include <vector>

#include <Json.hpp>

#include <common.h>

#include <MeterMap.hpp>
#include <Options.hpp>
#include <meter_protocol.hpp>

/**
 * General options from CLI
 */
class Config_Options {
public:
	Config_Options();
	Config_Options(const std::string filename);
	~Config_Options() {};

/**
 * Parse JSON formatted configuration file
 *
 * @param const char *filename the path of the configuration file
 * @param list_t *mappings a pointer to a list, where new channel<->meter mappings should be stored
 * @param config_options_t *options a pointer to a structure of global configuration options
 * @return int non-zero on success
 */
	void config_parse(MapContainer &mappings);
	void config_parse_meter(MapContainer &mappings, Json::Ptr jso);
	void config_parse_channel(Json& jso, MeterMap &metermap);

// getter
	const std::string &config() const { return _config; }
	const std::string &log() const { return _log; }
	FILE *logfd() { return _logfd; }
	const int &port()      const { return _port; }
	const int &verbosity() const { return _verbosity; }
	const int &comet_timeout() const { return _comet_timeout; }
	const int &buffer_length() const { return _buffer_length; }
	const int retry_pause() const { return _retry_pause; }

	const bool channel_index() const { return _channel_index; }
	const bool daemon()    const { return _daemon; }
	const bool foreground()const { return _foreground; }
	const bool local()     const { return _local; }
	const bool logging()   const { return _logging; }

	const bool doRegistration()   const { return _doRegistration; }

// setter
	void config(const std::string &v) { _config = v; }
	void log(const std::string &v)    { _log = v; }
	void logfd(FILE *fd)              { _logfd = fd; }
	void port(const int v)      { _port = v; }
	void verbosity(int v) { _verbosity = v; }

	void daemon(const bool v)    { _daemon = v; }
	void foreground(const bool v){ _foreground = v; }
	void local(const bool v)     { _local = v; }
	void logging(const bool v)    { _logging = v; }
	void doRegistration(const bool v)    { _doRegistration = v; }

private:
	std::string _config;		/* filename of configuration */
	std::string _log;		/* filename for logging */
	FILE *_logfd;

	int _port;		/* TCP port for local interface */
	int _verbosity;		/* verbosity level */
	int _comet_timeout;	/* in seconds;  */
	int _buffer_length;	/* in seconds; how long to buffer readings for local interfalce */
	int _retry_pause;	/* in seconds; how long to pause after an unsuccessful HTTP request */

	/* boolean bitfields, padding at the end of struct */
	int _channel_index:1;	/* give a index of all available channels via local interface */
	int _daemon:1;		/* run in background */
	int _foreground:1;	/* dont fork in background */
	int _local:1;		/* enable local interface */
	int _logging:1;		/* start logging threads, depends on local & daemon */
	int _doRegistration:1;		/* start logging threads, depends on local & daemon */
};

/**
 * Reads key, value and type from JSON object
 *
 * @param jso the JSON object
 * @param key the key of the option
 * @param option a pointer to a uninitialized option_t structure
 * @return 0 on succes, <0 on error
 */
//int option_init(struct json_object *jso, char *key,  Option *option);

/**
 * Validate UUID
 * Match against something like: '550e8400-e29b-11d4-a716-446655440000'
 *
 * @param const char *uuid a string containing to uuid
 * @return int non-zero on success
 */
int config_validate_uuid(const char *uuid);


#endif /* _CONFIG_H_ */
