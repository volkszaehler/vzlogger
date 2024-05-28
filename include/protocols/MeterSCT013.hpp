/**
 * Read SCT013 current sensor on Raspberry PICO via GPIO
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

#ifndef _METER_SCT013_H_
#define _METER_SCT013_H_

#include <protocols/Protocol.hpp>

class MeterSCT013 : public vz::protocol::Protocol {

  public:
    MeterSCT013(std::list<Option> options);
    virtual ~MeterSCT013();

    int open();
    int close();
    ssize_t read(std::vector<Reading> &rds, size_t n);

  private:
    uint  maxCurrent;   // SCT max current, e.g. 30
    uint  adcNum;       // Which ADC pin, starting with 0 from 26
    float offset;
    uint  adcPin;       // Resulting ADC pin
    uint  numSamples;   // How many GPIO ADC queries that will be averaged into one sample
    uint  delay;        // Pause between GPIO ADC queries
    float ratio;        // Calc current from measured voltage
    uint  cycles;
};

#endif /* _METER_SCT013_H_ */
