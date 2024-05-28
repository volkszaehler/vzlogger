/**
 * Read data from SCT013 via GPIO - valid on Pico only via analog pins
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
#include <math.h>

#include "Options.hpp"
#include "protocols/MeterSCT013.hpp"
#include <VZException.hpp>

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#define ADC_FIRST 26 // GPIO pin 26
#define ADC_VREF 3.3
#define ADC_RANGE (1 << 12)
#define ADC_CONVERT (ADC_VREF / (ADC_RANGE - 1))
#define GRID_VOLTAGE 230

MeterSCT013::MeterSCT013(std::list<Option> options) : Protocol("sct013")
{
  OptionList optlist;

  adcNum = 0; // Effectively use Pin 26 + 0
  numSamples = 1480; // One sample every ~15s
  delay = 10; // 100 per sec

  const char * optName;
  try
  {
    optName = "adcnum";     adcNum = optlist.lookup_int(options, optName);
    optName = "numsamples"; numSamples = optlist.lookup_int(options, optName);
    optName = "delay";      delay = optlist.lookup_int(options, optName);
    optName = "scttype";    maxCurrent = optlist.lookup_int(options, optName);
  }
  catch (vz::VZException &e)
  {
    print(log_alert, "Missing '%s' or invalid type", name().c_str(), optName);
    throw;
  }

  ratio = maxCurrent * (ADC_VREF / (ADC_RANGE));
  adcPin = ADC_FIRST + adcNum;
  offset = (ADC_RANGE >> 1);
  print(log_debug, "Created MeterSCT013 ADC %d (GPIO %d), sct %d, delay %d num %d", "", adcNum, adcPin, maxCurrent, delay, numSamples);
  cycles = 0;
}

MeterSCT013::~MeterSCT013() {}

int MeterSCT013::open()
{
  // Init
  adc_gpio_init(adcPin);
  print(log_debug, "Opened MeterSCT013", "");
  return SUCCESS;
}

int MeterSCT013::close()
{
  return SUCCESS;
}

ssize_t MeterSCT013::read(std::vector<Reading> &rds, size_t n)
{
  print(log_debug, "Reading MeterSCT013 at GPIO %d (ADC %d)", "", adcPin, adcNum);

  adc_select_input(adcNum);

  uint  adc_raw;
  float sum = 0;
  float filtered;
  float sq;
  for(uint i = 0; i < numSamples; i++)
  {
    adc_raw = adc_read(); // raw voltage from ADC

    // Inspired by EmonLib
    offset = (offset + (adc_raw - offset) / 1024);
    filtered = adc_raw - offset;
    sq = filtered * filtered;
    sum += sq;

    sleep_ms(delay);
  }

  float irms = ratio * sqrt(sum / numSamples);
  float watts = irms * GRID_VOLTAGE;

  print(log_debug, "MeterSCT013::read: irms %.2fA -> %.2fW (sum: %.2f off: %.2f)", "", irms, watts, sum, offset);
  print(log_debug, "MeterSCT013::read: %d, %d", "", rds.size(), n);

  if(cycles++ == 0)
  {
    // Seems as if the very first run always produces rubbish values ...
    return 0;
  }

  rds[0].value(irms);
  ReadingIdentifier *rid(new StringIdentifier("Current"));
  rds[0].identifier(rid);
  rds[0].time(); // use current timestamp

  rds[1].value(watts);
  ReadingIdentifier *rid2(new StringIdentifier("Power"));
  rds[1].identifier(rid2);
  rds[1].time(); // use current timestamp

  return 2;
}
