/**
 * Read data from HC-SR04 ultrasonic distance sensor
 * See https://github.com/dangarbri/pico-distance-sensor
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

#include <climits>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "Options.hpp"
#include "protocols/MeterHCSR04.hpp"
#include <VZException.hpp>
#include <distance_sensor.h>

#include <stdio.h>
#include "pico/stdlib.h"

MeterHCSR04::MeterHCSR04(std::list<Option> options) : Protocol("hc-sr04")
{
  ids[0] = ReadingIdentifier::Ptr(new StringIdentifier("Distance"));

  OptionList optlist;

  const char * optName;
  try
  {
    optName = "trigger"; trigger = optlist.lookup_int(options, optName);
    optName = "pioInst"; pioInst = optlist.lookup_int(options, optName);
  }
  catch (vz::VZException &e)
  {
    print(log_alert, "Missing '%s' or invalid type", name().c_str(), optName);
    throw;
  }

  print(log_debug, "Created MeterHCSR04 (GPIO %d)", "", trigger);
}

MeterHCSR04::~MeterHCSR04() {}

int MeterHCSR04::open()
{
  // Create an instance of the sensor
  // specify the pio, state machine, and gpio pin connected to trigger
  // echo pin must be on gpio pin trigger + 1.
  hcsr04 = new DistanceSensor(pioInst == 0 ? pio0 : pio1, 0, trigger);

  print(log_debug, "Opened MeterHCSR04 %p", "", hcsr04);
  return SUCCESS;
}

int MeterHCSR04::close()
{
  print(log_debug, "Closing MeterHCSR04 %p", "", hcsr04);
  delete hcsr04;
  return SUCCESS;
}

ssize_t MeterHCSR04::read(std::vector<Reading> &rds, size_t n)
{
  print(log_debug, "Reading MeterHCSR04 at GPIO %d", "", trigger);

  // Trigger background sense
  hcsr04->TriggerRead();

  // wait for sensor to get a result
  while (hcsr04->is_sensing)
  {
    sleep_us(100);
  }

  // Read result
  print(log_debug, "MeterHCSR04::read: Dist %dcm", "", hcsr04->distance);
  print(log_debug, "MeterHCSR04::read: %d, %d", "", rds.size(), n);

  rds[0].value(hcsr04->distance);
  rds[0].identifier(ids[0]);
  rds[0].time(); // use current timestamp

  return 1;
}

