/**
 * Read data from EmonLib - valid on Pico only via analog pins
 * See https://github.com/openenergymonitor/EmonLib/blob/master/EmonLib.cpp
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
#include "protocols/MeterEmonLib.hpp"
#include <VZException.hpp>

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#define ADC_FIRST 26 // GPIO pin 26
#define ADC_VREF 3.3
#define ADC_RANGE (1 << 12)

#define TIMEOUT 2000 // in msecs

MeterEmonLib::MeterEmonLib(std::list<Option> options) : Protocol("emonlib")
{
  ids[0] = ReadingIdentifier::Ptr(new StringIdentifier("Current"));
  ids[1] = ReadingIdentifier::Ptr(new StringIdentifier("ApparentPower"));
  ids[2] = ReadingIdentifier::Ptr(new StringIdentifier("Voltage"));
  ids[3] = ReadingIdentifier::Ptr(new StringIdentifier("RealPower"));
  ids[4] = ReadingIdentifier::Ptr(new StringIdentifier("PowerFactor"));

  OptionList optlist;

  adcCurrent = 0; // Effectively use Pin 26 + 0
  adcVoltage = 1000;
  numSamples = 1480; // One sample every ~15s
  delay = 0;

  const char * optName;
  try
  {
    optName = "adcCurrent"; adcCurrent = optlist.lookup_int(options, optName);
    optName = "numSamples"; numSamples = optlist.lookup_int(options, optName);
    optName = "delay";      delay = optlist.lookup_int(options, optName);
    optName = "currentCalibration"; iCal = optlist.lookup_int(options, optName);
    optName = "voltageCalibration"; vCal = optlist.lookup_double(options, optName);
  }
  catch (vz::VZException &e)
  {
    print(log_alert, "Missing '%s' or invalid type", name().c_str(), optName);
    throw;
  }

  try
  {
    optName = "adcVoltage";       adcVoltage = optlist.lookup_int(options, optName);
    optName = "phaseCalibration"; phaseCal = optlist.lookup_double(options, optName);
  }
  catch (vz::VZException &e)
  {
    print(log_info, "No voltage configured.", name().c_str(), optName);
  }

  adcPinI = ADC_FIRST + adcCurrent;
  emon.current(adcCurrent, iCal);       // Current: input pin, calibration.
  print(log_debug, "Created MeterEmonLib ADC %d (GPIO %d), sct %.2fA, delay %d num %d", "", adcCurrent, adcPinI, iCal, delay, numSamples);

  if(adcVoltage != 1000)
  {
    adcPinU = ADC_FIRST + adcVoltage;
    emon.voltage(adcVoltage, vCal, phaseCal);  // Voltage: input pin, calibration, phase_shift
    print(log_debug, "SubMeterEmonLib Voltage %.2fV ADC %d (GPIO %d)", "", vCal, adcVoltage, adcPinU);
  }

  cycles = 0;
}

MeterEmonLib::~MeterEmonLib() {}

int MeterEmonLib::open()
{
  // Init
  adc_gpio_init(adcPinI);
  if(adcVoltage != 1000)
  {
    adc_gpio_init(adcPinU);
  }

  print(log_debug, "Opened MeterEmonLib", "");
  return SUCCESS;
}

int MeterEmonLib::close()
{
  return SUCCESS;
}

ssize_t MeterEmonLib::read(std::vector<Reading> &rds, size_t n)
{
  return (adcVoltage == 1000) ? this->readIrms(rds, n) : this->readIV(rds, n);
}

/* -------------------------------------------------------------------------------------------
 * Read only Irms, calculate apparent power ("Scheinleistung")
 * ------------------------------------------------------------------------------------------ */

ssize_t MeterEmonLib::readIrms(std::vector<Reading> &rds, size_t n)
{
  print(log_debug, "Reading MeterEmonLib at GPIO %d (ADC %d)", "", adcPinI, adcCurrent);

  double irms = emon.calcIrms(numSamples);
  float watts = irms * vCal;

  print(log_debug, "MeterEmonLib::readIrms: irms %.2fA -> %.2fW", "", irms, watts);
  print(log_debug, "MeterEmonLib::readIrms: %d, %d", "", rds.size(), n);

  // Seems as if the very first cycles always produces rubbish values ...
  if(cycles++ < 4)
  {
    print(log_debug, "Ignoring because cycles: %d", "", (cycles - 1));
    return 0;
  }

  for(uint i = 0; i < 2; i++)
  {
    rds[i].value(i == 0 ? irms : watts);
    rds[i].identifier(ids[i]);
    rds[i].time(); // use current timestamp
  }

  return 2;
}

/* -------------------------------------------------------------------------------------------
 * Read both I and U, calculate power values
 * ------------------------------------------------------------------------------------------ */

ssize_t MeterEmonLib::readIV(std::vector<Reading> &rds, size_t n)
{
  print(log_debug, "Reading MeterEmonLib at GPIO %d (ADC %d), U: %d (%d)", "", adcPinI, adcCurrent, adcPinU, adcVoltage);

  emon.calcVI(numSamples, TIMEOUT, delay);       // Calculate all. No.of half wavelengths (crossings), time-out
  
  print(log_debug, "MeterEmonLib::readIV: irms %.2fA Voltage: %.2fV -> %.2fW (app: %.2fW, factor: %.2f)", "",
        emon.Irms, emon.Vrms, emon.realPower, emon.apparentPower, emon.powerFactor);

  // Seems as if the very first cycles always produces rubbish values ...
  if(cycles++ < 4)
  {
    print(log_debug, "Ignoring because cycles: %d", "", (cycles - 1));
    return 0;
  }

  for(uint i = 0; i < 5; i++)
  {
    switch(i)
    {
      case 0: rds[i].value(emon.Irms);          break;
      case 1: rds[i].value(emon.apparentPower); break;
      case 2: rds[i].value(emon.Vrms);          break;
      case 3: rds[i].value(emon.realPower);     break;
      case 4: rds[i].value(emon.powerFactor);   break;
    }
    rds[i].identifier(ids[i]);
    rds[i].time(); // use current timestamp
  }

  return 5;
}
