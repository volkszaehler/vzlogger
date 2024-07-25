/**
 * Thread implementations
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
#include <unistd.h>

#include "Reading.hpp"
#include "threads.h"
#include "vzlogger.h"
#include <ApiIF.hpp>
#include <api/InfluxDB.hpp>
#include <api/MySmartGrid.hpp>
#include <api/Null.hpp>
#include <api/Volkszaehler.hpp>
#ifdef LOCAL_SUPPORT
#include "local.h"
#endif
#ifdef ENABLE_MQTT
#include "mqtt.hpp"
#endif
#include "PushData.hpp"

extern Config_Options options;

void *reading_thread(void *arg) {
	MeterMap *mapping = static_cast<MeterMap *>(arg);
	Meter::Ptr mtr = mapping->meter();

	// Only allow cancellation at safe points
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	try {

		do { /* start thread main loop */
			_safe_to_cancel();

                     mapping->read();

		} while (true);
	} catch (std::exception &e) {
		std::stringstream oss;
		oss << e.what();
		print(log_alert, "Reading-THREAD - reading got an exception : %s", mtr->name(), e.what());
		pthread_exit(0);
	}

	print(log_debug, "Stopped reading. ", mtr->name());

	pthread_exit(0);
	return NULL;
}

void *logging_thread(void *arg) { // is started by Channel::start and stopped via
								  // Channel::cancel via pthread_cancel!
	Channel *__this =
		static_cast<Channel *>(arg);           // retrieve the pointer to the corresponding Channel
	Channel::Ptr ch = __this->_this_forthread; // And get a copy of the Channel owner's shared_ptr
											   // for passing it on.
	do { /* start thread mainloop */
		try {
			ch->wait();
                  ch->sendData(ch);
		} catch (std::exception &e) {
			print(log_alert, "Logging thread failed due to: %s", ch->name(), e.what());
		}

	} while (true); // endless?!

	print(log_debug, "Stopped logging.", ch->name());
	pthread_exit(0);

	return NULL;
}
