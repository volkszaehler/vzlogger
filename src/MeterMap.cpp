/***********************************************************************/
/** @file MeterMap.hpp
 *
 *  @author Kai Krueger
 *  @date   2012-03-13
 *  @email  kai.krueger@itwm.fraunhofer.de
 *
 * (C) Fraunhofer ITWM
 **/
/*---------------------------------------------------------------------*/

/**
 * Protocol interface
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
#include <math.h>

#include "threads.h"
#include <Config_Options.hpp>
#include <MeterMap.hpp>
#include <api/InfluxDB.hpp>
#include <api/MySmartGrid.hpp>
#include <api/Null.hpp>
#include <api/Volkszaehler.hpp>

extern Config_Options options; /* global application options */

/**
	If the meter is enabled, start the meter and all its channels.
*/
void MeterMap::start() {
	if (_meter->isEnabled()) {
		try {
			_meter->open();
		} catch (vz::ConnectionException &e) {
			if (_meter->skip()) {
				print(log_warning, "Skipping meter %s", NULL, _meter->name());
				return;
			} else {
				throw;
			}
		}

		print(log_info, "Meter connection established", _meter->name());
		pthread_create(&_thread, NULL, &reading_thread, (void *)this);
		print(log_debug, "Meter thread started", _meter->name());

		print(log_debug, "Meter is opened. Starting channels.", _meter->name());
		for (iterator it = _channels.begin(); it != _channels.end(); it++) {
			(*it)->start(*it);
			print(log_debug, "Logging thread started", (*it)->name());
		}
		_thread_running = true;
	} else {
		print(log_info, "Meter for protocol '%s' is disabled. Skipping.", _meter->name(),
			  _meter->protocol()->name().c_str());
	}
}

void MeterMap::cancel() { // is called from MapContainer::quit which is called from sigint handler
						  // handler ::quit
	print(log_finest, "MeterMap::cancel entered...", _meter->name());
	if (_meter->isEnabled() && running()) {
		for (iterator it = _channels.begin(); it != _channels.end(); it++) {
			(*it)->cancel(); // stops the logging_thread via pthread_cancel
			(*it)->join();
		}
		print(log_finest, "MeterMap::cancel wait for readingthread", _meter->name());
		pthread_cancel(_thread); // readingthread
		pthread_join(_thread, NULL);
		_thread_running = false;
		print(log_finest, "MeterMap::cancel wait for meter::close", _meter->name());
		_meter->close();
		//_channels.clear();
	}
	print(log_finest, "MeterMap::cancel finished.", _meter->name());
}

void MeterMap::registration() {
	// Channel::Ptr ch;

	if (!_meter->isEnabled()) {
		return;
	}
	for (iterator ch = _channels.begin(); ch != _channels.end(); ch++) {
		// create configured api interfaces
		// NOTE: if additional APIs are introduced both threads.cpp and MeterMap.cpp need to be
		// updated
		vz::ApiIF::Ptr api;
		if (0 == strcasecmp((*ch)->apiProtocol().c_str(), "mysmartgrid")) {
			api = vz::ApiIF::Ptr(new vz::api::MySmartGrid(*ch, (*ch)->options()));
			print(log_debug, "Using MySmartGrid api.", (*ch)->name());
		} else if (0 == strcasecmp((*ch)->apiProtocol().c_str(), "influxdb")) {
			api = vz::ApiIF::Ptr(new vz::api::InfluxDB(*ch, (*ch)->options()));
			print(log_debug, "Using InfluxDB api", (*ch)->name());
		} else if (0 == strcasecmp((*ch)->apiProtocol().c_str(), "null")) {
			api = vz::ApiIF::Ptr(new vz::api::Null(*ch, (*ch)->options()));
			print(log_debug, "Using null api - meter data available via local httpd if enabled.",
				  (*ch)->name());
		} else {
			if (strcasecmp((*ch)->apiProtocol().c_str(), "volkszaehler"))
				print(log_alert, "Wrong config! api: <%s> is unknown!", (*ch)->name(),
					  (*ch)->apiProtocol().c_str());
			// try to use volkszaehler api anyhow:
			api = vz::ApiIF::Ptr(new vz::api::Volkszaehler(*ch, (*ch)->options()));
			print(log_debug, "Using default volkszaehler api.", (*ch)->name());
		}

		api->register_device();
	}
	printf("..done\n");
}
