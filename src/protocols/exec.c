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

#include "meter.h"
#include "protocols/exec.h"
#include "options.h"

int meter_init_exec(meter_t *mtr, list_t options) {
	meter_handle_exec_t *handle = &mtr->handle.exec;

	char *command;
	if (options_lookup_string(options, "command", &command) == SUCCESS) {
		handle->command = strdup(command);
	}
	else {
		print(log_error, "Missing command or invalid type", mtr);
		return ERR;
	}

	char *format;
	switch (options_lookup_string(options, "format", &format)) {
		case SUCCESS:
			handle->format = strdup(format);
			// TODO parse format (see file.c)
			break;

		case ERR_NOT_FOUND:
			handle->format = NULL; /* use default format */
			break;

		default:
			print(log_error, "Failed to parse format", mtr);
			return ERR;
	}


	return SUCCESS;
}

void meter_free_exec(meter_t *mtr) {
	meter_handle_exec_t *handle = &mtr->handle.exec;

	free(handle->command);

	if (handle->format != NULL) {
		free(handle->format);
	}
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
