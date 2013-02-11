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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Options.hpp"
#include <VZException.hpp>


Option::Option(const char *pKey)
		: _key(pKey)
{
	//_key = strdup(pKey);
}

Option::Option(const char *pKey, struct json_object *jso)
		: _key(pKey)
{
//std::cout<< "New option...."<< pKey << std::endl;

	switch (json_object_get_type(jso)) {
			case json_type_string:	_value_string = json_object_get_string(jso);   break;
			case json_type_int:	    value.integer = json_object_get_int(jso);     break;
			case json_type_boolean:	value.boolean = json_object_get_boolean(jso); break;
			case json_type_double:	value.floating = json_object_get_double(jso); break;
			default:		throw vz::VZException("Not a valid Type");
	}

	_type = (type_t)json_object_get_type(jso);
}

Option::Option(const char *pKey, char *pValue)
		: _key(pKey)
		, _type(type_string)
		, _value_string(pValue)
{
//_key = strdup(pKey);
	//value.string = strdup(pValue);
}

Option::Option(const char *pKey, int pValue)
		: _key(pKey)
{
//_key = strdup(pKey);
	value.integer = pValue;
	_type = type_int;
}

Option::Option(const char *pKey, double pValue)
		: _key(pKey)
{
//_key = strdup(pKey);
	value.floating = pValue;
	_type = type_double;
}

Option::Option(const char *pKey, bool pValue)
		: _key(pKey)
{
//_key = strdup(pKey);
	value.boolean = pValue;
	_type = type_boolean;
}

Option::~Option() {
//	if (_key != NULL) {
//		free(_key);
//	}

	//if (value.string != NULL && _type == type_string) {
	//	free((void*)(value.string));
	//}
}

Option::operator const char *() const {
	if (_type != type_string) throw vz::InvalidTypeException("not a string");

	return _value_string.c_str();
}

Option::operator int() const {
	if (_type != type_int) throw vz::InvalidTypeException("Invalid type");

	return value.integer;
}

Option::operator double() const {
	if (_type != type_double) throw vz::InvalidTypeException("Invalid type");

	return value.floating;
}

Option::operator bool() const {
	if (_type != type_boolean) throw vz::InvalidTypeException("Invalid type");

	return value.boolean;
}

//Option& OptionList::lookup(List<Option> options, char *key) {
const Option &OptionList::lookup(std::list<Option> options, const std::string &key) {
	for(const_iterator it = options.begin(); it != options.end(); it++) {
		if ( it->key() == key ) {
			return (*it);
		}
	}

	throw vz::OptionNotFoundException("Option '"+ std::string(key) +"' not found");
}

const char *OptionList::lookup_string(std::list<Option> options, const char *key)
{
	Option opt = lookup(options, key);
	return (const char*)opt;
}

const int OptionList::lookup_int(std::list<Option> options, const char *key)
{
	Option opt = lookup(options, key);
	return (int)opt;
}

const bool OptionList::lookup_bool(std::list<Option> options, const char *key)
{
	Option opt = lookup(options, key);
	return (bool)opt;
}

const double OptionList::lookup_double(std::list<Option> options, const char *key)
{
	Option opt = lookup(options, key);
	return (double)opt;
}

void OptionList::dump(std::list<Option> options) {
	std::cout<< "OptionList dump\n" ;

	for(const_iterator it = options.begin(); it != options.end(); it++) {
		std::cout << (*it) << std::endl;
	}
}
