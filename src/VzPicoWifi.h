/**
 * Raspberry Pico WiFi class
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

#ifndef VZ_PICO_WIFI
#define VZ_PICO_WIFI

#include "vz_pico_inline_config.h"
#include <string>
#include <common.h>

class VzPicoWifi
{
  public:
    VzPicoWifi(uint numRetries = 20);
    ~VzPicoWifi();

    bool init();
    bool enable();
    void disable();
    time_t getSysRefTime();
    bool isLinkUp();

  private:
    uint    retries;
    time_t  sysRefTime;
    bool    firstTime;
};

#endif // VZ_PICO_WIFI
