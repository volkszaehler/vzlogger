/**
 * Read data from onboard temp via GPIO - valid on Pico only via analog pins
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
#include "protocols/MeterOnboardTemp.hpp"
#include <VZException.hpp>

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#define ADC_FIRST 26 // GPIO pin 26

MeterOnboardTemp::MeterOnboardTemp(std::list<Option> options) : Protocol("onboardTemp")
{
  OptionList optlist;

  const char * optName;
  try
  {
    optName = "unit"; unit = (optlist.lookup_string(options, optName))[0];
  }
  catch (vz::VZException &e)
  {
    print(log_alert, "Missing '%s' or invalid type", "", optName);
    throw;
  }

  if(! ( unit == 'C') || (unit == 'F'))
  {
    print(log_alert, "Unsupported temperature unit '%s'", "", unit);
    // TODO throw
  } 

  print(log_debug, "MeterOnboardTemp created. Unit: '%c'", "", unit);
}

MeterOnboardTemp::~MeterOnboardTemp() {}

int MeterOnboardTemp::open()
{
  // Init
  print(log_debug, "MeterOnboardTemp enable temp sensor ADC", "");
  adc_set_temp_sensor_enabled(true);
  adcNum = 4; // Hardcoded
  print(log_debug, "MeterOnboardTemp opened. ADC '%d'", "", adcNum);
  return SUCCESS;
}

int MeterOnboardTemp::close()
{
  print(log_debug, "MeterOnboardTemp closing", "");
  return SUCCESS;
}

ssize_t MeterOnboardTemp::read(std::vector<Reading> &rds, size_t n)
{
  adc_select_input(adcNum);

  print(log_debug, "MeterOnboardTemp reading ... ADC '%d'", "", adcNum);

  /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
  const float conversionFactor = 3.3f / (1 << 12);

  float adc = (float)adc_read() * conversionFactor;
  float temp = 27.0f - (adc - 0.706f) / 0.001721f;

  if (unit == 'F')
  {
    temp = temp * 9 / 5 + 32;
  }

  print(log_debug, "MeterOnboardTemp reading ... temp: %fC", "", temp);
  print(log_debug, "MeterOnboardTemp::read: %d, %d", "", rds.size(), n);

  rds[0].value(temp);
  ReadingIdentifier *rid(new NilIdentifier());
  rds[0].identifier(rid);
  rds[0].time(); // use current timestamp

  return 1;
}
