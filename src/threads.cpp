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

#include "reading.h"
#include "vzlogger.h"
#include "threads.h"
#include <ApiIF.hpp>
#include <api/Volkszaehler.hpp>
#include <api/MySmartGrid.hpp>

extern Config_Options options;

void reading_thread_cleanup(void *rds) {
	free(rds);
}

void * reading_thread(void *arg) {
	std::vector<Reading> rds;
	MeterMap *mapping = static_cast<MeterMap *>(arg);
	Meter::Ptr  mtr = mapping->meter();
	time_t last, delta;
	const meter_details_t *details;
	size_t n = 0;

	details = meter_get_details(mtr->protocolId());

	/* allocate memory for readings */
	//size_t bytes = sizeof(reading_t) * details->max_readings;
	//rds = malloc(bytes);
	//memset(rds, 0, bytes);
  for(size_t i=0; i< details->max_readings; i++) {
    Reading rd(mtr->identifier());
    rds.push_back(rd);
  }
  
	//pthread_cleanup_push(&reading_thread_cleanup, rds);

  print(log_debug, "Number of readers: %d", mtr->name(), details->max_readings);
  print(log_debug, "Config.daemon: %d", "", options.daemon());
  print(log_debug, "Config.local: %d", "", options.local());


  do { /* start thread main loop */
		/* fetch readings from meter and calculate delta */
		last = time(NULL);
		n = mtr->read(rds, details->max_readings);
		delta = time(NULL) - last;

		/* dumping meter output */
		if (options.verbosity() > log_debug) {
			print(log_debug, "Got %i new readings from meter:", mtr->name(), n);

			char identifier[MAX_IDENTIFIER_LEN];
			for (size_t i = 0; i < n; i++) {
				rds[i].unparse(mtr->protocolId(), identifier, MAX_IDENTIFIER_LEN);
				print(log_debug, "Reading: id=%s/%s value=%.2f ts=%.3f", mtr->name(),
              identifier, rds[i].identifier()->toString().c_str(),
              rds[i].value(), rds[i].tvtod());
			}
		}

		/* update buffer length with current interval */
		if (details->periodic == FALSE && delta > 0 && delta != mtr->interval()) {
			print(log_debug, "Updating interval to %i", mtr->name(), delta);
			mtr->interval(delta);
		}

		/* insert readings into channel queues */
    for(MeterMap::iterator ch = mapping->begin(); ch!=mapping->end(); ch++) {
			Reading *add = NULL;

      print(log_debug, "Check channel %s, n=%d", mtr->name(), ch->name(), n);
      
			for (size_t i = 0; i < n; i++) {
        print(log_debug, "Search channel (%d - %s)", mtr->name(), i, ch->name());
        print(log_debug, "lhs-id=%s, rhr=%s", mtr->name(),
              rds[i].identifier()->toString().c_str(),
              ch->identifier()->toString().c_str());
        
				if ( *rds[i].identifier().get() == *ch->identifier().get()) {
          print(log_debug, "found channel", mtr->name());
					if (ch->tvtod() < rds[i].tvtod()) {
						ch->last(&rds[i]);
					}

					print(log_info, "Adding reading to queue (value=%.2f ts=%.3f)", ch->name(),
                rds[i].value(), rds[i].tvtod());
					ch->push(rds[i]);

					if (add == NULL) {
						add = &rds[i]; /* remember first reading which has been added to the buffer */
					}
				}
			}

			/* update buffer length */
			if (options.local()) {
				//buf->keep = (mtr->interval > 0) ? ceil(options.buffer_length / mtr->interval) : 0;
			}

			/* queue reading into sending buffer logging thread if
			   logging is enabled & sent queue is empty */
			if (options.logging()) {
				//ch->push(buf->sent = add);
			}

			/* shrink buffer */
			//buffer_clean(buf);

			/* notify webserver and logging thread */
			ch->notify();

			/* debugging */
			if (options.verbosity() >= log_debug) {
				size_t dump_len = 24;
				char *dump = (char*)malloc(dump_len);

				if (dump == NULL) {
					print(log_error, "cannot allocate buffer", ch->name());
				}

				while (dump == NULL || ch->dump(dump, dump_len) == NULL) {
					dump_len *= 1.5;
					free(dump);
					dump = (char*)malloc(dump_len);
				}

				print(log_debug, "Buffer dump (size=%i keep=%i): %s", ch->name(),
              ch->size(), ch->keep(), dump);

				free(dump);
			}
		}

		if ((options.daemon() || options.local()) && details->periodic) {
			print(log_info, "Next reading in %i seconds", mtr->name(), mtr->interval());
			sleep(mtr->interval());
		}
  } while (options.daemon() || options.local());

  print(log_debug, "Stop reading.! ", mtr->name());
	//pthread_cleanup_pop(1);

	return NULL;
}

void logging_thread_cleanup(void *arg) {
//	api_handle_t *api = (api_handle_t *) arg;

//	api_free(api);
}

void * logging_thread(void *arg) {
	Channel::Ptr ch;
  ch.reset(static_cast<Channel *>(arg)); /* casting argument */

  // create configured api-interface
  vz::ApiIF::Ptr api;
  if( ch->apiName() == "mysmartgrid") {
    api =  vz::ApiIF::Ptr(new vz::api::MySmartGrid(ch));
    print(log_debug, "Using MSG-Api.", ch->name());
  } else {
    api =  vz::ApiIF::Ptr(new vz::api::Volkszaehler(ch));
    print(log_debug, "Using default api:", ch->name());
  }

	//pthread_cleanup_push(&logging_thread_cleanup, &api);

	do { /* start thread mainloop */

		ch->wait();

    api->send();
    
	} while (options.daemon());

  print(log_debug, "Stop logging.! ", ch->name());
	//pthread_cleanup_pop(1);

	return NULL;
}
