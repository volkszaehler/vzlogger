/***********************************************************************/
/** @file InfluxDB.cpp
 * Header file for InfluxDB API calls
 *
 * @author Stefan Kuntz
 * @email  Stefan.github@gmail.com
 * @copyright Copyright (c) 2017, The volkszaehler.org project
 * @package vzlogger
 * @license http://opensource.org/licenses/gpl-license.php GNU Public License
 **/
/*---------------------------------------------------------------------*/

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
#include <curl/curl.h>
#include "Config_Options.hpp"
#include <api/InfluxDB.hpp>
#include <VZException.hpp>
extern Config_Options options;

vz::api::InfluxDB::InfluxDB(
	Channel::Ptr ch,
	std::list<Option> pOptions
	)
	: ApiIF(ch)
{
	OptionList optlist;
	print(log_info, "InfluxDB API initialize", ch->name());

// config file options
try {
		_host = optlist.lookup_string(pOptions, "host");
		print(log_finest, "api InfluxDB using host %s", ch->name(), _host.c_str());
	} catch (vz::OptionNotFoundException &e) {
		print(log_error, "api InfluxDB requires parameter \"host\"!", ch->name());
		throw;
	} catch (vz::VZException &e) {
		print(log_error, "api InfluxDB requires parameter \"host\" as string!", ch->name());
		throw;
	}

try {
		_username = optlist.lookup_string(pOptions, "username");
		print(log_finest, "api InfluxDB using username %s",ch->name(), _username.c_str());
	} catch (vz::OptionNotFoundException &e) {
		//print(log_error, "api InfluxDB requires parameter \"username\"!", ch->name());
		//throw;
		print(log_debug, "api InfluxDB no username set", ch->name());
		_username = "";
	} catch (vz::VZException &e) {
		print(log_error, "api InfluxDB requires parameter \"username\" as string!", ch->name());
		throw;
	}

try {
		_password = optlist.lookup_string(pOptions, "password");
		//TODO: remove logging of password
		print(log_finest, "api InfluxDB using password %s", ch->name(), _password.c_str());
	} catch (vz::OptionNotFoundException &e) {
		//print(log_error, "api InfluxDB requires parameter \"password\"!", ch->name());
		//throw;
		print(log_debug, "api InluxDB no password set", ch->name());
		_password = "";
	} catch (vz::VZException &e) {
		print(log_error, "api InfluxDB requires parameter \"password\" as string!", ch->name());
		throw;
	}

try {
		_database = optlist.lookup_string(pOptions, "database");
		print(log_finest, "api InfluxDB using database %s", ch->name(), _database.c_str());
	} catch (vz::OptionNotFoundException &e) {
		print(log_debug, "api InfluxDB will use default database \"vzlogger\"", ch->name());
		_database = "vzlogger";
	} catch (vz::VZException &e) {
		print(log_error, "api InfluxDB requires parameter \"database\" as string!", ch->name());
		throw;
	}

try {
		_measurement_name = optlist.lookup_string(pOptions, "measurement_name");
		print(log_finest, "api InfluxDB using measurement name %s", ch->name(), _database.c_str());
	} catch (vz::OptionNotFoundException &e) {
		print(log_debug, "api InfluxDB will use default measurement name \"vzlogger\"", ch->name());
		_measurement_name = "vzlogger";
	} catch (vz::VZException &e) {
		print(log_error, "api InfluxDB requires parameter \"measurement_name\" as string!", ch->name());
		throw;
	}
}

vz::api::InfluxDB::~InfluxDB() // destructor
{
}

void vz::api::InfluxDB::send()
{
	// we need to mark all elements as transmitted/deleted otherwise the Channel::Buffer keeps on growing
	Buffer::Ptr buf = channel()->buffer();
	buf->lock();
	for (	Buffer::iterator it = buf->begin(); it != buf->end(); it++) {
		it->mark_delete();
	}
	buf->unlock();
	buf->clean();
}

void vz::api::InfluxDB::register_device()
{
}


/*
 * Local variables:
 *  tab-width: 2
 *  c-indent-level: 2
 *  c-basic-offset: 2
 *  project-name: vzlogger
 * End:
 */
