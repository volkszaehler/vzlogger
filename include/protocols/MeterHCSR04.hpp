/**
 * Read HC-SR04 distance sensor on Raspberry PICO via GPIO
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

#ifndef _METER_HCSR04_H_
#define _METER_HCSR04_H_

#include <protocols/Protocol.hpp>

class DistanceSensor;

class MeterHCSR04 : public vz::protocol::Protocol
{
  public:
    MeterHCSR04(std::list<Option> options);
    virtual ~MeterHCSR04();

    int open();
    int close();
    ssize_t read(std::vector<Reading> &rds, size_t n);

  private:
    DistanceSensor * hcsr04;

    uint trigger;  // Which GPIO pin, starting with 0 from 26
    uint pioInst;  // Which PIO instance: 0 or 1

    ReadingIdentifier::Ptr ids[1];
};

#endif /* _METER_HCSR04_H_ */
