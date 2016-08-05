/**
 * Thread implementations
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
#include <unistd.h>

#include "Reading.hpp"
#include "vzlogger.h"
#include "threads.h"
#include <ApiIF.hpp>
#include <api/Volkszaehler.hpp>
#include <api/MySmartGrid.hpp>
#include <api/Null.hpp>
#ifdef LOCAL_SUPPORT
#include "local.h"
#endif
#include "PushData.hpp"

extern Config_Options options;

void reading_thread_cleanup(void *rds) {
	free(rds);
}

void * reading_thread(void *arg) {
	std::vector<Reading> rds;
	MeterMap *mapping = static_cast<MeterMap *>(arg);
	Meter::Ptr  mtr = mapping->meter();
	time_t aggIntEnd;
	const meter_details_t *details;
	size_t n = 0;

	details = meter_get_details(mtr->protocolId());

	/* allocate memory for readings */
	for (size_t i=0; i< details->max_readings; i++) {
		Reading rd(mtr->identifier());
		rds.push_back(rd);
	}

	//pthread_cleanup_push(&reading_thread_cleanup, rds);

	print(log_debug, "Number of readers: %d", mtr->name(), details->max_readings);
	print(log_debug, "Config.daemon: %d", mtr->name(), options.daemon());
	print(log_debug, "Config.local: %d", mtr->name(), options.local());


	try {
		aggIntEnd = time(NULL);
		do { /* start thread main loop */
			aggIntEnd += mtr->aggtime(); /* end of this aggregation period */
			do { /* aggregate loop */
				/* fetch readings from meter and calculate delta */
				n = mtr->read(rds, details->max_readings);

				/* dumping meter output */
				if (options.verbosity() > log_debug) {
					print(log_debug, "Got %i new readings from meter:", mtr->name(), n);

					char identifier[MAX_IDENTIFIER_LEN];
					for (size_t i = 0; i < n; i++) {
						rds[i].unparse(/*mtr->protocolId(),*/ identifier, MAX_IDENTIFIER_LEN);
						print(log_debug, "Reading: id=%s/%s value=%.2f ts=%lld", mtr->name(),
								identifier, rds[i].identifier()->toString().c_str(),
								rds[i].value(), rds[i].time_ms());
					}
				}

				/* update buffer length with current interval */
//				if (details->periodic == FALSE && delta > 0 && delta != mtr->interval()) {
//					print(log_debug, "Updating interval to %i", mtr->name(), delta);
//					mtr->interval(delta);
//				}

				/* insert readings into channel queues */
				if (n>0)
				for (MeterMap::iterator ch = mapping->begin(); ch!=mapping->end(); ch++) {
					Reading *add = NULL;

					//print(log_debug, "Check channel %s, n=%d", mtr->name(), ch->name(), n);

					for (size_t i = 0; i < n; i++) {
						if (*rds[i].identifier().get() == *(*ch)->identifier().get()) {
							int div = (*ch)->interval_div(1);
							if (div == 0) {
								//print(log_debug, "found channel", mtr->name());
								if ((*ch)->time_ms() < rds[i].time_ms()) {
									(*ch)->last(&rds[i]);
								}

								print(log_info, "Adding reading to queue (value=%.2f ts=%lld)", (*ch)->name(),
										rds[i].value(), rds[i].time_ms());
								(*ch)->push(rds[i]);
							// provide data to push data server:
							if (pushDataList) {
								const std::string uuid = (*ch)->uuid();
								pushDataList->add(uuid, rds[i].time_ms(), rds[i].value());
								print(log_finest, "added to uuid %s", "push", uuid.c_str());
							}
							} else {
								print(log_info, "Skipping [%d/%d] reading    (value=%.2f ts=%lld)", (*ch)->name(),
										div, (*ch)->interval_factor(), rds[i].value(), rds[i].time_ms());
							}


							if (add == NULL) {
								add = &rds[i]; /* remember first reading which has been added to the buffer */
							}
						}
					}

				} // channel loop
			} while((mtr->aggtime() > 0) && (time(NULL) < aggIntEnd)); /* default aggtime is -1 */

			for (MeterMap::iterator ch = mapping->begin(); ch!=mapping->end(); ch++) {

				/* aggregate buffer values if aggmode != NONE */
				(*ch)->buffer()->aggregate(mtr->aggtime(), mtr->aggFixedInterval());
				/* mark buffer "ready" */
				(*ch)->buffer()->have_newValues();

				/* shrink buffer */
				(*ch)->buffer()->clean();
#ifdef LOCAL_SUPPORT
				if (options.local()) {
					shrink_localbuffer(); // remove old/outdated data in the local buffer
					add_ch_to_localbuffer(*(*ch)); // add this ch data to the local buffer
				}
#endif
				/* notify webserver and logging thread */
				(*ch)->notify();

				/* debugging */
				if (options.verbosity() >= log_debug) {
					size_t dump_len = 24;
					char *dump = (char*)malloc(dump_len);

					if (dump == NULL) {
						print(log_error, "Cannot allocate buffer", (*ch)->name());
					}

					while (dump == NULL || (*ch)->dump(dump, dump_len) == NULL) {
						dump_len *= 1.5;
						free(dump);
						dump = (char*)malloc(dump_len);
					}

					print(log_debug, "Buffer dump (size=%i): %s", (*ch)->name(),
							(*ch)->size(), dump);

					free(dump);
				}

				// if logging is not enabled we need to empty the ch->buffer here already:
				if (!options.logging()) {
					(*ch)->buffer()->clean(false);
				}
			}

			if (mtr->interval() > 0) {
				print(log_info, "Next reading in %i seconds", mtr->name(), mtr->interval());
				sleep(mtr->interval());
			}
		} while (options.daemon() || options.local() || options.logging() );
	} catch (std::exception &e) {
		std::stringstream oss;
		oss << e.what();
		print(log_error, "Reading-THREAD - reading got an exception : %s", mtr->name(), e.what());
		pthread_exit(0);
	}

	print(log_debug, "Stopped reading. ", mtr->name());
	//pthread_cleanup_pop(1);

	pthread_exit(0);
	return NULL;
}

void logging_thread_cleanup(/*void *arg*/) {
//	api_handle_t *api = (api_handle_t *) arg;

//	api_free(api);
}

void * logging_thread(void *arg) { // get's started from Channel::start and stopped via Channel::cancel via pthread_cancel!
	Channel::Ptr ch;
	ch.reset(static_cast<Channel *>(arg)); /* casting argument */
	print(log_debug, "Start logging thread for %s-api. Running as daemon: %s", ch->name(),
				ch->apiProtocol().c_str(), options.daemon() ? "yes" : "no");

	// create configured api interfaces
	// NOTE: if additional APIs are introduced both threads.cpp and MeterMap.cpp need to be updated
	vz::ApiIF::Ptr api;
	if (0 == strcasecmp(ch->apiProtocol().c_str(), "mysmartgrid")) {
		api =  vz::ApiIF::Ptr(new vz::api::MySmartGrid(ch, ch->options()));
		print(log_debug, "Using MySmartGrid api.", ch->name());
	}
	else if (0 == strcasecmp(ch->apiProtocol().c_str(), "null")) {
		api =  vz::ApiIF::Ptr(new vz::api::Null(ch, ch->options()));
		print(log_debug, "Using null api- meter data available via local httpd if enabled.", ch->name());
	} else {
		if (strcasecmp(ch->apiProtocol().c_str(), "volkszaehler"))
			print(log_error, "Wrong config! api: <%s> is unknown!", ch->name(), ch->apiProtocol().c_str());
		// try to use volkszaehler api anyhow:

		// default == volkszaehler
		api =  vz::ApiIF::Ptr(new vz::api::Volkszaehler(ch, ch->options()));
		print(log_debug, "Using default volkszaehler api.", ch->name());
	}

	//pthread_cleanup_push(&logging_thread_cleanup, &api);

	do { /* start thread mainloop */
		try {
			ch->wait();
			api->send();
		}
		catch (std::exception &e) {
			print(log_error, "Logging thread failed due to: %s", ch->name(), e.what());
		}

	} while (options.logging());

	print(log_debug, "Stopped logging. (daemon=%d)", ch->name(), options.daemon());
	pthread_exit(0);
	//pthread_cleanup_pop(1);

	return NULL;
}
