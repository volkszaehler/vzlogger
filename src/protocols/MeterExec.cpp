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

#include <stdlib.h>
#include <sys/time.h>

#include "protocols/MeterExec.hpp"
#include "Options.hpp"
#include <VZException.hpp>

MeterExec::MeterExec(std::list<Option> options) 
		: Protocol("exec")
{
	OptionList optlist;

	try {
		_command = optlist.lookup_string(options, "command");
	} catch( vz::VZException &e ) {
		print(log_error, "Missing command or invalid type", "");
		throw;
	}

	try {
		_format = optlist.lookup_string(options, "format");
	} catch( vz::OptionNotFoundException &e ) {
		_format = NULL; /* use default format */
	} catch( vz::VZException &e ) {
		print(log_error, "Failed to parse format", "");
		throw;
	}
}

MeterExec::~MeterExec() {

	free((void*)_command);

	if (_format != NULL) {
		free((void*)_format);
	}
}

int MeterExec::open() {

	// TODO implement
	return ERR;
}

int MeterExec::close() {

	// TODO implement
	return ERR;
}

ssize_t MeterExec::read(std::vector<Reading> &rds, size_t n) {
	//meter_handle_exec_t *handle = &mtr->handle.exec;

	// TODO implement
	return 0;
}
