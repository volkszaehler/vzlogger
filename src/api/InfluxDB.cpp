/***********************************************************************/
/** @file InfluxDB.cpp
 * Header file for null API calls
 *
 * @author Andreas Goetz
 * @date   2012-03-15
 * @email  cpuidle@gmx.de
 * @copyright Copyright (c) 2014, The volkszaehler.org project
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

try {
		_host = optlist.lookup_string(pOptions, "middleware");
	} catch (vz::OptionNotFoundException &e) {
		print(log_error, "api InfluxDB requires parameter \"host\"!", ch->name());
		throw;
	} catch (vz::VZException &e) {
		print(log_error, "api InfluxDB requires parameter \"host\" as string!", ch->name());
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
