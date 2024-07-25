/**
 * Read EmonLib current sensor on Raspberry PICO via GPIO
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

#ifndef _METER_EMONLIB_H_
#define _METER_EMONLIB_H_

#include <protocols/Protocol.hpp>
#include <EmonLib.h>

class MeterEmonLib : public vz::protocol::Protocol
{
  public:
    MeterEmonLib(std::list<Option> options);
    virtual ~MeterEmonLib();

    int open();
    int close();
    ssize_t read(std::vector<Reading> &rds, size_t n);

  private:
    EnergyMonitor emon;

    uint  maxCurrent;   // SCT max current, e.g. 30A
    uint  adcCurrent;   // Which ADC pin, starting with 0 from 26
    uint  adcVoltage;   // Which ADC pin, starting with 0 from 26
    uint  adcPinI;      // Resulting ADC pin (current)
    uint  adcPinU;      // Resulting ADC pin (voltage)
    uint  numSamples;   // How many GPIO ADC queries that will be averaged into one sample
    uint  delay;        // Pause between GPIO ADC queries
    uint  cycles;
    float iCal, vCal, phaseCal;

    // StringIdentifier ids[5];
    // ReadingIdentifier * ids[5];
    const char * ids[5];

    ssize_t readIrms(std::vector<Reading> &rds, size_t n);
    ssize_t readIV(std::vector<Reading> &rds, size_t n);
};

#endif /* _METER_EMONLIB_H_ */
