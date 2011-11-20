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

#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>

#include "meter.h"
#include "file.h"
#include "options.h"

int meter_init_file(meter_t *mtr, list_t options) {
	meter_handle_file_t *handle = &mtr->handle.file;

	char *path;
	if (options_lookup_string(options, "path", &path) == SUCCESS) {
		handle->path = strdup(path);
	}
	else {
		print(log_error, "Missing path or invalid type", mtr);
		return ERR;
	}

	char *regex;
	if (options_lookup_string(options, "regex", &regex) == SUCCESS) {
		handle->regex = strdup(regex);
	}
	else if (options_lookup_string(options, "regex", &regex) == ERR_INVALID_TYPE) { // TODO improve code
		print(log_error, "Regex has to be a string", mtr);
		return ERR;
	}

	return SUCCESS;
}

int meter_open_file(meter_t *mtr) {
	meter_handle_file_t *handle = &mtr->handle.file;

	handle->fd = fopen(handle->path, "r");

	if (handle->fd == NULL) {
		print(log_error, "fopen(%s): %s", mtr, handle->path, strerror(errno));
		return ERR;
	}

	return SUCCESS;
}

int meter_close_file(meter_t *mtr) {
	meter_handle_file_t *handle = &mtr->handle.file;

	return fclose(handle->fd);
}

size_t meter_read_file(meter_t *mtr, reading_t rds[], size_t n) {
	meter_handle_file_t *handle = &mtr->handle.file;

	char buffer[16];
	int bytes;

	rewind(handle->fd);
	bytes = fread(buffer, 1, 16, handle->fd);
	buffer[bytes] = '\0'; /* zero terminated, required? */

	if (bytes) {
		rds->value = strtof(buffer, NULL);
		gettimeofday(&rds->time, NULL);

		return 1;
	}

	return 0; /* empty file */
}
