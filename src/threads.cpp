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

inline void _safe_to_cancel() {
	// see https://blog.memzero.de/pthread-cancel-noexcept/
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_testcancel();
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
}

inline void _cancellable_sleep(int seconds) {
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	sleep(seconds);
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
}

void *reading_thread(void *arg) {
	MeterMap *mapping = static_cast<MeterMap *>(arg);
	Meter::Ptr mtr = mapping->meter();
	time_t aggIntEnd;
	const meter_details_t *details;
	size_t n = 0;
	bool first_reading = true;

	// Only allow cancellation at safe points
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	details = meter_get_details(mtr->protocolId());
	std::vector<Reading> rds(details->max_readings, Reading(mtr->identifier()));

	print(log_debug, "Number of readers: %d", mtr->name(), details->max_readings);
	print(log_debug, "Config.local: %d", mtr->name(), options.local());

	try {
		aggIntEnd = time(NULL);
		do { /* start thread main loop */
			_safe_to_cancel();
			do {
				aggIntEnd += mtr->aggtime(); /* end of this aggregation period */
			} while ((aggIntEnd < time(NULL)) && (mtr->aggtime() > 0));
			do { /* aggregate loop */
				_safe_to_cancel();
				int interval = mtr->interval();
				if (interval > 0 && !first_reading) {
					print(log_info, "waiting %i seconds before next reading", mtr->name(),
						  interval);
					_cancellable_sleep(interval);
				}
				first_reading = false;

				/* fetch readings from meter and calculate delta */
				n = mtr->read(rds, details->max_readings);

				/* dumping meter output */
				if (options.verbosity() > log_debug) {
					print(log_debug, "Got %i new readings from meter:", mtr->name(), n);

					char identifier[MAX_IDENTIFIER_LEN];
					for (size_t i = 0; i < n; i++) {
						rds[i].unparse(/*mtr->protocolId(),*/ identifier, MAX_IDENTIFIER_LEN);
						print(log_debug, "Reading: id=%s/%s value=%.2f ts=%lld", mtr->name(),
							  identifier, rds[i].identifier()->toString().c_str(), rds[i].value(),
							  rds[i].time_ms());
					}
				}
				if (n > 0 && !options.haveTimeMachine())
					for (size_t i = 0; i < n; i++)
						if (rds[i].time_s() < 631152000) { // 1990-01-01 00:00:00
							print(log_error,
								  "meter returned readings with a timestamp before 1990, IGNORING.",
								  mtr->name());
							print(log_error, "most likely your meter is misconfigured,",
								  mtr->name());
							print(log_error,
								  "for sml meters, set `\"use_local_time\": true` in vzlogger.conf"
								  " (meter section),",
								  mtr->name());
							print(log_error,
								  "to override this check, set `\"i_have_a_time_machine\": true`"
								  " in vzlogger.conf.",
								  mtr->name());
							// note: we do NOT throw an exception or such,
							// because this might be a spurious error,
							// the next reading might be valid again.
							n = 0;
						}

				/* insert readings into channel queues */
				if (n > 0)
					for (MeterMap::iterator ch = mapping->begin(); ch != mapping->end(); ch++) {

						// print(log_debug, "Check channel %s, n=%d", mtr->name(), ch->name(), n);

						for (size_t i = 0; i < n; i++) {
							if (*rds[i].identifier().get() == *(*ch)->identifier().get()) {
								// print(log_debug, "found channel", mtr->name());
								if ((*ch)->time_ms() < rds[i].time_ms()) {
									(*ch)->last(&rds[i]);
								}

								print(log_info, "Adding reading to queue (value=%.2f ts=%lld)",
									  (*ch)->name(), rds[i].value(), rds[i].time_ms());
								(*ch)->push(rds[i]);

								// provide data to push data server:
								if (pushDataList) {
									const std::string uuid = (*ch)->uuid();
									pushDataList->add(uuid, rds[i].time_ms(), rds[i].value());
									print(log_finest, "added to uuid %s", "push", uuid.c_str());
								}
#ifdef ENABLE_MQTT
								// update mqtt values as well:
								if (mqttClient) {
									mqttClient->publish((*ch), rds[i]);
								}
#endif
							}
						}

					}                                                   // channel loop
			} while ((mtr->aggtime() > 0) && (time(NULL) < aggIntEnd)); /* default aggtime is -1 */

			for (MeterMap::iterator ch = mapping->begin(); ch != mapping->end(); ch++) {

				/* aggregate buffer values if aggmode != NONE */
				(*ch)->buffer()->aggregate(mtr->aggtime(), mtr->aggFixedInterval());
				/* mark buffer "ready" */
				(*ch)->buffer()->have_newValues();

				/* shrink buffer */
				(*ch)->buffer()->clean();
#ifdef LOCAL_SUPPORT
				if (options.local()) {
					shrink_localbuffer();          // remove old/outdated data in the local buffer
					add_ch_to_localbuffer(*(*ch)); // add this ch data to the local buffer
				}
#endif
#ifdef ENABLE_MQTT
				// update mqtt values as well:
				if (mqttClient) {
					Buffer::Ptr buf = (*ch)->buffer();
					Buffer::iterator it;
					buf->lock();
					for (it = buf->begin(); it != buf->end(); ++it) {
						if (&*it) { // this seems dirty. see issue #427
									// the lock()/unlock() should avoid it.
							Reading &r = *it;
							if (!r.deleted()) {
								mqttClient->publish((*ch), r, true);
							}
						}
					}
					buf->unlock();
				}
#endif

				/* notify webserver and logging thread */
				(*ch)->notify();

				/* debugging */
				if (options.verbosity() >= log_debug) {
					// print(log_debug, "Buffer dump (size=%i): %s", (*ch)->name(),
					//(*ch)->size(), (*ch)->dump().c_str());
				}
			}
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
	print(log_debug, "Start logging thread for %s-api.", ch->name(), ch->apiProtocol().c_str());

	// create configured api interfaces
	// NOTE: if additional APIs are introduced both threads.cpp and MeterMap.cpp need to be updated
	vz::ApiIF::Ptr api;
	if (0 == strcasecmp(ch->apiProtocol().c_str(), "mysmartgrid")) {
		api = vz::ApiIF::Ptr(new vz::api::MySmartGrid(ch, ch->options()));
		print(log_debug, "Using MySmartGrid api.", ch->name());
	} else if (0 == strcasecmp(ch->apiProtocol().c_str(), "influxdb")) {
		api = vz::ApiIF::Ptr(new vz::api::InfluxDB(ch, ch->options()));
		print(log_debug, "Using InfluxDB api", ch->name());
	} else if (0 == strcasecmp(ch->apiProtocol().c_str(), "null")) {
		api = vz::ApiIF::Ptr(new vz::api::Null(ch, ch->options()));
		print(log_debug, "Using null api- meter data available via local httpd if enabled.",
			  ch->name());
	} else {
		if (strcasecmp(ch->apiProtocol().c_str(), "volkszaehler"))
			print(log_alert, "Wrong config! api: <%s> is unknown!", ch->name(),
				  ch->apiProtocol().c_str());
		// try to use volkszaehler api anyhow:

		// default == volkszaehler
		api = vz::ApiIF::Ptr(new vz::api::Volkszaehler(ch, ch->options()));
		print(log_debug, "Using default volkszaehler api.", ch->name());
	}

	do { /* start thread mainloop */
		try {
			ch->wait();
			api->send();
		} catch (std::exception &e) {
			print(log_alert, "Logging thread failed due to: %s", ch->name(), e.what());
		}

	} while (true); // endless?!

	print(log_debug, "Stopped logging.", ch->name());
	pthread_exit(0);

	return NULL;
}
