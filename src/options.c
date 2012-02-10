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

Option::Option(char *pKey, char *pValue) { Option(pKey);
	value.string = strdup(pValue);
	type = type_string;
}

Option::Option(char *pKey, int pValue) { Option(pKey);
	value.integer = pValue;
	type = type_int;
}

Option::Option(char *pKey, double pValue) { Option(pKey);
	value.floating = pValue;
	type = type_double;
}

Option::Option(char *pKey, bool pValue) { Option(pKey);
	value.boolean = pValue;
	type = type_boolean;
}

Option::Option(char *pKey) {
	key = strdup(pKey);
}

Option::~Option() {
	if (key != NULL) {
		free(key);
	}

	if (value.string != NULL && type == type_string) {
		free(value.string);
	}
}

Option::operator (char *)() {
	if (type != type_string) throw Exception("Invalid type");

	return value.string;
}

Option::operator int() {
	if (type != type_int) throw Exception("Invalid type");

	return value.integer;
}

Option::operator double() {
	if (type != type_double) throw Exception("Invalid type");

	return value.floating;
}

Option::operator bool() {
	if (type != type_boolean) throw Exception("Invalid type");

	return value.boolean;
}

Option& OptionList::lookup(List<Option> options, char *key) {
	Option &option;

	/* linear search */
	foreach(options, val, option_t) {
		if (strcmp(val->key, key) == 0) {
			option = val;
		}
	}

	if (option == options.end()) {
		throw Exception("Option not found");
	}

	return option;
}

