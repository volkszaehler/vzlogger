/**
 * Get data by calling a program
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
// Regex is not working with gcc-4.6
//#include <regex>
//#include <string>

#include "Options.hpp"
#include "protocols/MeterExec.hpp"
#include <VZException.hpp>
#include <sys/types.h>
#include <unistd.h>

MeterExec::MeterExec(std::list<Option> options) : Protocol("exec") {
	OptionList optlist;

	try {
		_command = optlist.lookup_string(options, "command");
	} catch (vz::VZException &e) {
		print(log_alert, "MeterExec::MeterExec: Missing command or invalid type", name().c_str());
		throw;
	}

	// an optional format string for scanf()
	try {
		const char *config_format = optlist.lookup_string(options, "format");

		/**
		 * Compiling the provided format string in a format string for scanf
		 * by replacing the following tokens
		 *
		 * "$v" => "%1$lf" (value)
		 * "$i" => "%2$ms" (identifier)		(memory gets allocated by sscanf())
		 * "$t" => "%3$lf" (timestamp)
		 */

		int config_len = strlen(config_format);
		int scanf_len =
			config_len +
			15; // adding extra space for longer conversion specification in scanf_format

		char *scanf_format = (char *)malloc(scanf_len); // the scanf format string

		int i = 0; // index in config_format string
		int j = 0; // index in scanf_format string
		while (i <= config_len && j <= scanf_len) {
			switch (config_format[i]) {
			case '$':
				if (i + 1 < config_len) { // introducing a token
					switch (config_format[i + 1]) {
					case 'v':
						j += sprintf(scanf_format + j, "%%1$lf");
						break;
					case 'i':
						j += sprintf(scanf_format + j, "%%2$ms");
						break;
					case 't':
						j += sprintf(scanf_format + j, "%%3$lf");
						break;
					default:
						break;
						// Regex is not working with gcc-4.6
						// (?<identifier>[\w]+): (?<value>[\d]+[,|.]?[?:\d]+) (?<timestamp>\d{10})
						// case 'v': j += sprintf(scanf_format+j, "([\\d]+[.|,]?[?:\\d]+)"); break;
						// case 'i': j += sprintf(scanf_format+j, "([\\w]+)"); break;
						// case 't': j += sprintf(scanf_format+j, "(\\d{10})"); break;
					}
					i++;
				}
				break;

			case '%':
				scanf_format[j++] = '%'; // add double %% to escape a conversion identifier
										 // fallthrough
			default:
				scanf_format[j++] = config_format[i]; // just copying
			}

			i++;
		}

		print(log_debug, "MeterExec::MeterExec: Parsed format string \"%s\" => \"%s\"",
			  name().c_str(), config_format, scanf_format);
		_format = scanf_format;
	} catch (vz::OptionNotFoundException &e) {
		_format = ""; // use default format
	} catch (vz::VZException &e) {
		print(log_alert, "MeterExec::MeterExec: Failed to parse format", name().c_str());
		throw;
	}
}

MeterExec::~MeterExec() {}

int MeterExec::open() {
#ifndef METEREXEC_ROOTACCESS
	if (geteuid() == 0) {
		print(log_alert, "MeterExec::open: MeterExec protocol cannot be run with root privileges!",
			  name().c_str());
		print(log_alert,
			  "                 If you really want this, compile vzlogger with:", name().c_str());
		print(log_alert, "                 'cmake -D METEREXEC_ROOTACCESS=true .'", name().c_str());
		return ERR;
	}
#else
	print(log_info, "MeterExec::open: MeterExec protocol is compiled with root privileges!",
		  name().c_str());
#endif

	print(log_debug, "MeterExec::open: Executing command line '%s'", name().c_str(), command());
	_pipe = popen(command(), "r");

	if (_pipe == NULL) {
		print(log_alert, "MeterExec::open: popen(%s) failed with: %s", name().c_str(), command(),
			  strerror(errno));
		return ERR;
	}

	if (_pipe != NULL) {
		pclose(_pipe);
		_pipe = NULL;
	}

	return SUCCESS;
}

int MeterExec::close() { return SUCCESS; }

ssize_t MeterExec::read(std::vector<Reading> &rds, size_t n) {
	char buffer[256];
	char *string = 0;

	unsigned int i = 0;

	print(log_debug, "MeterExec::read: Calling '%s'", name().c_str(), command());
	_pipe = popen(command(), "r");

	if (_pipe) {
		while (i < n && !feof(_pipe)) {
			while (i < n && fgets(buffer, 256, _pipe)) {
				char *nl;
				if ((nl = strrchr(buffer, '\n')))
					*nl = '\0'; // remove trailing newlines
				if ((nl = strrchr(buffer, '\r')))
					*nl = '\0';

				if (_format != "") {
					double timestamp = -1.0;

					// at least the value has to be read
					double value = 0.0;

					print(log_debug, "MeterExec::read: Reading line: '%s'", name().c_str(), buffer);
					int found = sscanf(buffer, format(), &value, &string, &timestamp);
					print(log_debug, "MeterExec::read: string: %s, value: %lf, timestamp: %lf",
						  name().c_str(), string ? string : "<null>", value, timestamp);
					// Regex is not working with gcc-4.6
					// std::smatch result;
					// std::string input(buffer);
					// std::regex regex(format());
					// bool found = regex_match(input, result, regex);
					// print(log_debug, "MeterExec::read: input: %s, value: %lf, string: %s,
					// timestamp: %lf", name().c_str(), result[0], result[1], result[2], result[3]);

					rds[i].value(value);
					ReadingIdentifier *rid(new StringIdentifier(string ? string : "<null>"));
					rds[i].identifier(rid);
					if (found >= 1) {
						// Regex is not working with gcc-4.6
						// if (found) {
						if (timestamp >= 0.0)
							rds[i].time_from_double(timestamp);
						else
							rds[i].time(); // use current timestamp
						i++;               // read successfully
					}
					if (string) {
						free(string);
						string = 0;
					}
				} else { // just reading a value per line
					rds[i].value(strtod(buffer, NULL));
					rds[i].time();
					ReadingIdentifier *rid(new StringIdentifier(""));
					rds[i].identifier(rid);

					//					if (endptr != line) {
					i++; // read successfully
						 //					}
				}
			}
		}

		print(log_debug, "MeterExec::read: Closing process '%s'", name().c_str(), command());
		pclose(_pipe);
	} else { // _pipe == NULL
		print(log_warning, "MeterExec::read: popen(%s) failed with: %s", name().c_str(), command(),
			  strerror(errno));
	}

	return i;
}
