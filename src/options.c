/**
 * Option list functions
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

#include <string.h>

#include "options.h"
#include "common.h"

int options_lookup(list_t options, char *key, void *value, option_type_t type) {
	option_t * option = NULL;

	/* linear search */
	foreach(options, val, option_t) {
		if (strcmp(val->key, key) == 0) {
			option = val;
		}
	}

	/* checks */
	if (option == NULL) {
		return ERR_NOT_FOUND;
	}
	else if (option->type == type) {
		size_t n = 0;
		switch (option->type) {
			case option_type_boolean:	n = sizeof(option->value.integer); break; /* boolean is a bitfield: int boolean:1; */
			case option_type_double:	n = sizeof(option->value.floating); break;
			case option_type_int:	n = sizeof(option->value.integer); break;
			case option_type_string:	n = sizeof(option->value.string); break;
		}
		memcpy(value, &option->value, n);
		return SUCCESS;
	}
	else {
		return ERR_INVALID_TYPE;
	}
}

int options_lookup_boolean(list_t o, char *k, int *v)   { return options_lookup(o, k, v, option_type_boolean); }
int options_lookup_int(list_t o, char *k, int *v)       { return options_lookup(o, k, v, option_type_int); }
int options_lookup_string(list_t o, char *k, char **v)  { return options_lookup(o, k, v, option_type_string); }
int options_lookup_double(list_t o, char *k, double *v) { return options_lookup(o, k, v, option_type_double); }
