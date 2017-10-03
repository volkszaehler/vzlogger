/**
 * Read data from files & fifos
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>

#include "protocols/MeterFile.hpp"
#include "Options.hpp"
#include <VZException.hpp>

MeterFile::MeterFile(std::list<Option> options)
		: Protocol("file")
{
	OptionList optlist;

	try {
		_path = optlist.lookup_string(options, "path");
	} catch (vz::VZException &e) {
		print(log_alert, "Missing path or invalid type", name().c_str());
		throw;
	}

	// a optional format string for scanf()
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
		int scanf_len = config_len + 15; // adding extra space for longer conversion specification in scanf_format

		char *scanf_format = (char *)malloc(scanf_len); // the scanf format string

		int i = 0; // index in config_format string
		int j = 0; // index in scanf_format string
		while (i <= config_len && j <= scanf_len) {
			switch (config_format[i]) {
					case '$':
						if (i+1 < config_len) { // introducing a token
							switch (config_format[i+1]) {
									case 'v': j += sprintf(scanf_format+j, "%%1$lf"); break;
									case 'i': j += sprintf(scanf_format+j, "%%2$ms"); break;
									case 't': j += sprintf(scanf_format+j, "%%3$lf"); break;
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

		print(log_debug, "Parsed format string \"%s\" => \"%s\"", name().c_str(), config_format, scanf_format);
		_format = scanf_format;
	} catch (vz::OptionNotFoundException &e) {
		_format = ""; // use default format
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse format", name().c_str());
		throw;
	}

	// should we start each time at the beginning of the file?
	// or do we read from a logfile (append)
	try {
		_rewind = optlist.lookup_bool(options, "rewind");
	} catch (vz::OptionNotFoundException &e) {
		_rewind = FALSE; // do not rewind file by default
	} catch (vz::InvalidTypeException &e) {
		print(log_alert, "Invalid type for 'rewind'", name().c_str());
		throw;
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse 'rewind'", name().c_str());
		throw;
	}
}

MeterFile::~MeterFile() {
}

int MeterFile::open() {

	_fd = fopen(path(), "r");

	if (_fd == NULL) {
		print(log_alert, "fopen(%s): %s", name().c_str(), path(), strerror(errno));
		return ERR;
	}

	return SUCCESS;
}

int MeterFile::close() {

	return fclose(_fd);
}

ssize_t MeterFile::read(std::vector<Reading> &rds, size_t n) {

	// TODO use inotify to block reading until file changes

	char line[256], *endptr;
	char *string=0;

	// reset file pointer to beginning of file
	if (_rewind) {
		rewind(_fd);
	}

	unsigned int i = 0;
	print(log_debug, "MeterFile::read: %d, %d", "", rds.size(), n);

	while (i<n && fgets(line, 256, _fd)) {
		char *nl;
		if ((nl = strrchr(line, '\n'))) *nl = '\0'; // remove trailing newlines
		if ((nl = strrchr(line, '\r'))) *nl = '\0';

		if (_format != "") {
			double timestamp=-1.0;

			// at least the value has to been read
			double value=0.0;

			print(log_debug, "MeterFile::read: '%s'", "", line);
			int found = sscanf(line, format(), &value, &string, &timestamp);
			print(log_debug, "MeterFile::read: %lf, %s, %lf", "", value, string? string : "<null>", timestamp);


			rds[i].value(value);
			ReadingIdentifier *rid(new StringIdentifier(string ? string : "<null>"));
			rds[i].identifier(rid);
			if (found >= 1) {
				if (timestamp >=0.0)
					rds[i].time_from_double(timestamp);
				else
					rds[i].time(); // use current timestamp
				i++; // read successfully
			}
			if (string){
				free(string);
				string = 0;
			}
		}
		else { // just reading a value per line
			rds[i].value(strtod(line, &endptr));
			rds[i].time();
			ReadingIdentifier* rid(new StringIdentifier(""));
			rds[i].identifier(rid);

			if (endptr != line) {
				i++; // read successfully
			}
		}
	}

	return i;
}
