/**
 * Get data by calling programs
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

#include "meter.h"
#include "exec.h"
#include "options.h"

int meter_init_exec(meter_t *mtr, list_t options) {
	meter_handle_exec_t *handle = &mtr->handle.exec;

	if (options_lookup_string(options, "command", &handle->command) != SUCCESS) {
		print(log_error, "Missing command or invalid type", mtr);
		return ERR;
	}

	handle->regex = NULL;
	if (options_lookup_string(options, "regex", &handle->regex) == ERR_INVALID_TYPE) {
		print(log_error, "Regex has to be a string", mtr);
		return ERR;
	}

	return SUCCESS;
}

int meter_open_exec(meter_t *mtr) {
	//meter_handle_exec_t *handle = &mtr->handle.exec;

	// TODO implement
	return ERR;
}

int meter_close_exec(meter_t *mtr) {
	//meter_handle_exec_t *handle = &mtr->handle.exec;

	// TODO implement
	return ERR;
}

size_t meter_read_exec(meter_t *mtr, reading_t rds[], size_t n) {
	//meter_handle_exec_t *handle = &mtr->handle.exec;

	// TODO implement
	return 0;
}
