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

#ifdef LOCAL_SUPPORT
# include "local.h"
#endif // LOCAL_SUPPORT

#include <Config_Options.hpp>
#include <ApiIF.hpp>
#include <MeterMap.hpp>

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
#ifdef VZ_USE_THREADS
		pthread_create(&_thread, NULL, &reading_thread, (void *)this);
		print(log_debug, "Meter thread started", _meter->name());

		print(log_debug, "Meter is opened. Starting channels.", _meter->name());
		for (iterator it = _channels.begin(); it != _channels.end(); it++) {
			(*it)->start(*it);
			print(log_debug, "Logging thread started", (*it)->name());
		}
		_thread_running = true;
#else // VZ_USE_THREADS
  lastRead = 0;
#endif // VZ_USE_THREADS
	} else {
		print(log_info, "Meter for protocol '%s' is disabled. Skipping.", _meter->name(),
			  _meter->protocol()->name().c_str());
	}
  accTimeRead = 0;
  accTimeSend = 0;
  numUsed = 0;
}

void MeterMap::cancel() { // is called from MapContainer::quit which is called from sigint handler
						  // handler ::quit
	print(log_finest, "MeterMap::cancel entered...", _meter->name());
#ifdef VZ_USE_THREADS
	if (_meter->isEnabled() && running())
#else  // VZ_USE_THREADS
	if (_meter->isEnabled())
#endif // VZ_USE_THREADS
        {
#ifdef VZ_USE_THREADS
		for (iterator it = _channels.begin(); it != _channels.end(); it++) {
			(*it)->cancel(); // stops the logging_thread via pthread_cancel
			(*it)->join();
		}
		print(log_finest, "MeterMap::cancel wait for readingthread", _meter->name());
		pthread_cancel(_thread); // readingthread
		pthread_join(_thread, NULL);
		_thread_running = false;
#endif // VZ_USE_THREADS
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
		// updated
		vz::ApiIF::Ptr api = (*ch)->connect(*ch);
		api->register_device();
	}
	printf("..done\n");
}

void MeterMap::read()
{
  time_t tStart = time(NULL);

  Meter::Ptr mtr = this->meter();
  time_t aggIntEnd;
  const meter_details_t * details = meter_get_details(mtr->protocolId());
  size_t n = 0;

  std::vector<Reading> rds(details->max_readings, Reading(mtr->identifier()));

  print(log_debug, "Max number of readings: %d", mtr->name(), details->max_readings);
  print(log_debug, "Config.local: %d", mtr->name(), options.local());

  aggIntEnd = time(NULL);

  do
  {
    aggIntEnd += mtr->aggtime(); /* end of this aggregation period */
  } while ((aggIntEnd < time(NULL)) && (mtr->aggtime() > 0));

  do
  {
#ifdef VZ_USE_THREADS
    _safe_to_cancel();
    int interval = mtr->interval();
    if (interval > 0 && !first_reading)
    {
      print(log_info, "waiting %i seconds before next reading", mtr->name(), interval);
      _cancellable_sleep(interval);
    }
    first_reading = false;
#endif // VZ_USE_THREADS

    print(log_debug, "Querying meter ...", mtr->name());

    /* aggregate loop */
    /* fetch readings from meter and calculate delta */
    n = mtr->read(rds, details->max_readings);

    print(log_debug, "Got %i new readings from meter:", mtr->name(), n);

    /* dumping meter output */
    if (options.verbosity() > log_debug)
    {
      char identifier[MAX_IDENTIFIER_LEN];
      for (size_t i = 0; i < n; i++)
      {
        rds[i].unparse(/*mtr->protocolId(),*/ identifier, MAX_IDENTIFIER_LEN);
        print(log_debug, "Reading: id=%s/%s value=%.2f ts=%lld", mtr->name(),
              identifier, rds[i].identifier()->toString().c_str(), rds[i].value(),
              rds[i].time_ms());
      }
    }

    if (n > 0 && !options.haveTimeMachine())
    {
      for (size_t i = 0; i < n; i++)
      {
        if (rds[i].time_s() < 631152000)
        {
          // 1990-01-01 00:00:00
          print(log_error, "meter returned readings with a timestamp before 1990, IGNORING.", mtr->name());
          print(log_error, "most likely your meter is misconfigured,", mtr->name());
          print(log_error, "for sml meters, set `\"use_local_time\": true` in vzlogger.conf"
                " (meter section),", mtr->name());
          print(log_error, "to override this check, set `\"i_have_a_time_machine\": true`"
                " in vzlogger.conf.", mtr->name());
          // note: we do NOT throw an exception or such,
          // because this might be a spurious error,
          // the next reading might be valid again.
          n = 0;
        }
      }
    }

    /* insert readings into channel queues */
    if (n > 0)
    {
      for (MeterMap::iterator ch = this->begin(); ch != this->end(); ch++)
      {
        print(log_debug, "Check channel %s, n=%d", mtr->name(), (*ch)->name(), n);

        for (size_t i = 0; i < n; i++)
        {
          // printf("TGE Rd ID %s\n", (*rds[i].identifier().get()).toString().c_str());
          // printf("TGE Ch ID %s\n", (*(*ch)->identifier().get()).toString().c_str());
          if (*rds[i].identifier().get() == *(*ch)->identifier().get())
          {
            // print(log_debug, "found channel", mtr->name());
            if ((*ch)->time_ms() < rds[i].time_ms())
            {
              (*ch)->last(&rds[i]);
            }

            print(log_info, "Adding reading to queue (value=%.2f ts=%lld)",
                  (*ch)->name(), rds[i].value(), rds[i].time_ms());
            (*ch)->push(rds[i]);

#ifndef VZ_PICO
            // provide data to push data server:
            if (pushDataList)
            {
              const std::string uuid = (*ch)->uuid();
              pushDataList->add(uuid, rds[i].time_ms(), rds[i].value());
              print(log_finest, "added to uuid %s", "push", uuid.c_str());
            }
#endif // VZ_PICO
#ifdef ENABLE_MQTT
            // update mqtt values as well:
            if (mqttClient)
            {
              mqttClient->publish((*ch), rds[i]);
            }
#endif
          }
        }
      } // channel loop
    }
  } while ((mtr->aggtime() > 0) && (time(NULL) < aggIntEnd)); /* default aggtime is -1 */

  print(log_debug, "Reading data complete. Publishing ...", mtr->name());

#ifndef VZ_PICO
  // Sending from here not on RPi Pico - will be called from main loop
  this->sendData();
#endif // VZ_PICO

#ifndef VZ_USE_THREADS
  lastRead = time(NULL);
#endif // not VZ_USE_THREADS

  accTimeRead += (time(NULL) - tStart);
  numUsed++;
}

#ifndef VZ_USE_THREADS
int MeterMap::isDueIn()
{
  int due = meter()->interval() - (time(NULL) - lastRead);
  if(due <= 0)
  {
    print(log_debug, "Meter is due.", meter()->name());
  }
  else
  {
    print(log_finest, "Meter is not due - in %dsecs ....", meter()->name(), due);
  }
  return due;
}

bool MeterMap::readyToSend()
{
  print(log_finest, "Checking for sendable meter data ...", meter()->name());
  for (MeterMap::iterator ch = this->begin(); ch != this->end(); ch++)
  {
    uint numSamples = (*ch)->size();
    if(numSamples > 0)
    {
      print(log_debug, "%d readings ready for sending.",(*ch)->name(), numSamples);
      return true;
    }
  }
  print(log_finest, "No waiting data, not sending ...", meter()->name());
  return false;
}

bool MeterMap::isBusy()
{
  for (MeterMap::iterator ch = this->begin(); ch != this->end(); ch++)
  {
    if((*ch)->isBusy())
    {
      print(log_debug, "Network I/O busy.", meter()->name());
      return true;
    }
  }
  print(log_finest, "Network not busy.", meter()->name());
  return false;
}
#endif // not VZ_USE_THREADS

void MeterMap::sendData()
{
  time_t tStart = time(NULL);
  print(log_debug, "Sending data ...", meter()->name());
  for (MeterMap::iterator ch = this->begin(); ch != this->end(); ch++)
  {
    /* aggregate buffer values if aggmode != NONE */
    (*ch)->buffer()->aggregate(meter()->aggtime(), meter()->aggFixedInterval());
    /* mark buffer "ready" */
    (*ch)->buffer()->have_newValues();

    /* shrink buffer */
    (*ch)->buffer()->clean();
#ifdef LOCAL_SUPPORT
    if (options.local())
    {
      shrink_localbuffer();          // remove old/outdated data in the local buffer
      add_ch_to_localbuffer(*(*ch)); // add this ch data to the local buffer
    }
#endif
#ifdef ENABLE_MQTT
    // update mqtt values as well:
    if (mqttClient)
    {
      Buffer::Ptr buf = (*ch)->buffer();
      Buffer::iterator it;
      buf->lock();
      for (it = buf->begin(); it != buf->end(); ++it)
      {
        if (&*it)
        {
          // this seems dirty. see issue #427
          // the lock()/unlock() should avoid it.
          Reading &r = *it;
          if (!r.deleted())
          {
            mqttClient->publish((*ch), r, true);
          }
        }
      }
      buf->unlock();
    }
#endif

#ifdef VZ_USE_THREADS
    /* notify webserver and logging thread */
    (*ch)->notify();
#else // not VZ_USE_THREADS
    print(log_debug, "Sending %d readings to channel ...", (*ch)->name(), (*ch)->size());
    (*ch)->sendData(*ch);
#endif // VZ_USE_THREADS

    /* debugging */
    if (options.verbosity() >= log_debug)
    {
      // print(log_debug, "Buffer dump (size=%i): %s", (*ch)->name(),
      //(*ch)->size(), (*ch)->dump().c_str());
    }
  }
  accTimeSend += (time(NULL) - tStart);
  print(log_debug, "All meter data sent.", meter()->name());
}

void MeterMap::printStatistics(log_level_t logLevel)
{
  print(logLevel, "Read %d times, spent %ds reading, %ds sending", meter()->name(), numUsed, accTimeRead, accTimeSend);
}

