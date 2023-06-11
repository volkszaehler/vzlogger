/**
 * Option list functions
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
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

#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Options.hpp"
#include <VZException.hpp>

Option::Option(const char *pKey) : _key(pKey) {
	//_key = strdup(pKey);
}

Option::Option(const char *pKey, struct json_object *jso) : _key(pKey) {
	// std::cout<< "New option...."<< pKey << std::endl;

	switch (json_object_get_type(jso)) {
	case json_type_string:
		_value_string = json_object_get_string(jso);
		break;
	case json_type_int:
		value.integer = json_object_get_int(jso);
		break;
	case json_type_boolean:
		value.boolean = json_object_get_boolean(jso);
		break;
	case json_type_double:
		value.floating = json_object_get_double(jso);
		break;
	case json_type_object:
		value.jso = json_object_get(jso);
		break;
	case json_type_array:
		value.jso = json_object_get(jso);
		break;
	default: {
		std::stringstream s;
		s << std::string("Option::Option not a valid type ");
		s << pKey << " " << json_object_get_type(jso);
		throw vz::VZException(s.str());
	}
	}

	_type = (type_t)json_object_get_type(jso);
}

Option::Option(const Option &o) : _key(o._key), _type(o._type), _value_string(o._value_string) {
	if (_type == type_array || _type == type_object) {
		value.jso = json_object_get(o.value.jso);
	} else {
		value = o.value;
	}
}

Option::Option(const char *pKey, const char *pValue)
	: _key(pKey), _type(type_string), _value_string(pValue) {}

Option::Option(const char *pKey, const std::string &pValue)
	: _key(pKey), _type(type_string), _value_string(pValue) {}

Option::Option(const char *pKey, int pValue) : _key(pKey) {
	value.integer = pValue;
	_type = type_int;
}

Option::Option(const char *pKey, double pValue) : _key(pKey) {
	value.floating = pValue;
	_type = type_double;
}

Option::Option(const char *pKey, bool pValue) : _key(pKey) {
	value.boolean = pValue;
	_type = type_boolean;
}

Option::~Option() {
	if (_type == type_array || _type == type_object) {
		if (value.jso)
			json_object_put(value.jso);
		value.jso = 0;
	}
}

Option::operator const char *() const {
	if (_type != type_string)
		throw vz::InvalidTypeException("not a string");

	return _value_string.c_str();
}

Option::operator int() const {
	if (_type != type_int)
		throw vz::InvalidTypeException("Invalid type");

	return value.integer;
}

Option::operator double() const {
	if (_type != type_double)
		throw vz::InvalidTypeException("Invalid type");

	return value.floating;
}

Option::operator bool() const {
	if (_type != type_boolean)
		throw vz::InvalidTypeException("Invalid type");

	return value.boolean;
}

Option::operator struct json_object *() const {
	if (_type != type_array && _type != type_object)
		throw vz::InvalidTypeException("json_object not an array/object");
	return value.jso;
}

// Option& OptionList::lookup(List<Option> options, char *key) {
const Option &OptionList::lookup(std::list<Option> const &options, const std::string &key) const {
	for (const_iterator it = options.cbegin(); it != options.cend(); ++it) {
		if (it->key() == key) {
			return (*it);
		}
	}

	throw vz::OptionNotFoundException("Option '" + std::string(key) + "' not found");
}

const char *OptionList::lookup_string(std::list<Option> const &options, const char *key) const {
	const Option &opt = lookup(options, key);
	return opt.operator const char *();
}

const char *OptionList::lookup_string_tolower(std::list<Option> const &options,
											  const char *key) const {
	std::string str = lookup_string(options, key);
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str.c_str();
}

int OptionList::lookup_int(std::list<Option> const &options, const char *key) const {
	const Option &opt = lookup(options, key);
	return (int)opt;
}

bool OptionList::lookup_bool(std::list<Option> const &options, const char *key) const {
	const Option &opt = lookup(options, key);
	return (bool)opt;
}

double OptionList::lookup_double(std::list<Option> const &options, const char *key) const {
	const Option &opt = lookup(options, key);
	return (double)opt;
}

struct json_object *OptionList::lookup_json_array(std::list<Option> const &options,
												  const char *key) const {
	const Option &opt = lookup(options, key);
	if (opt.type() != Option::type_array)
		throw vz::InvalidTypeException("json_object not an array");
	return (struct json_object *)opt;
}

struct json_object *OptionList::lookup_json_object(std::list<Option> const &options,
												   const char *key) const {
	const Option &opt = lookup(options, key);
	if (opt.type() != Option::type_object)
		throw vz::InvalidTypeException("json_object not an object");
	return (struct json_object *)opt;
}

void OptionList::dump(std::list<Option> const &options) {
	std::cout << "OptionList dump\n";

	for (const_iterator it = options.begin(); it != options.end(); it++) {
		std::cout << (*it) << std::endl;
	}
}
