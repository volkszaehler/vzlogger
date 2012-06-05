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
#include <math.h>

#include <MeterMap.hpp>
#include <Config_Options.hpp>
#include <api/Volkszaehler.hpp>
#include <api/MySmartGrid.hpp>

extern Config_Options options;	/* global application options */

/**
	 If the meter is enabled, start the meter and all its channels.
*/
void MeterMap::start() {
	if(_meter->isEnabled()) {
		_meter->open();
		print(log_info, "Meter connection established", _meter->name());
		pthread_create(&_thread, NULL, &reading_thread, (void *) this);
		print(log_debug, "Meter thread started", _meter->name());

		print(log_debug, "meter is opened. Start channels.", _meter->name());
		for(iterator it = _channels.begin(); it!=_channels.end(); it++) {
			/* set buffer length for perriodic meters */
			if (meter_get_details(_meter->protocolId())->periodic && options.local()) {
				(*it)->buffer()->keep(ceil(options.buffer_length() / (double) _meter->interval()));
			}

			if (options.logging()) {
				(*it)->start();
				print(log_debug, "Logging thread started", (*it)->name());
			}
		}
		_thread_running = true;
	} else {
		print(log_info, "Meter for protocol '%s' is disabled. Skipping.", _meter->name(),
					_meter->protocol()->name().c_str());
	}

}
bool MeterMap::stopped() {
	if(_meter->isEnabled()  && running() ) {
		if( pthread_join(_thread, NULL) == 0 ) {
			_thread_running = false;

			// join channel-threads
			for(iterator it = _channels.begin(); it!=_channels.end(); it++) {
				(*it)->cancel();
				(*it)->join();
			}
			return true;
		}
	}
	return false;
}

void MeterMap::cancel() {
	if(_meter->isEnabled() && running() ) {
		for(iterator it = _channels.begin(); it!=_channels.end(); it++) {
			(*it)->cancel();
			(*it)->join();
		}
		pthread_cancel(_thread);
		pthread_join(_thread, NULL);
		_thread_running = false;

		//_channels.clear();
	}
}

void MeterMap::registration() {
	//Channel::Ptr ch;

	if( !_meter->isEnabled()) {
		return;
	}
	for(iterator ch = _channels.begin(); ch!=_channels.end(); ch++) {
    // create configured api-interface
		vz::ApiIF::Ptr api;
		if( (*ch)->apiProtocol() == "mysmartgrid") {
			api =  vz::ApiIF::Ptr(new vz::api::MySmartGrid(*ch, (*ch)->options()));
			print(log_debug, "Using MSG-Api.", (*ch)->name());
		} else {
			api =  vz::ApiIF::Ptr(new vz::api::Volkszaehler(*ch, (*ch)->options()));
			print(log_debug, "Using default api:", (*ch)->name());
		}

		//
		api->register_device();
	}
	printf("..done\n");
}
