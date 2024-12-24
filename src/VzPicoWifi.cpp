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

#include "pico/cyw43_arch.h"
#include "VzPicoWifi.h"
#include <Ntp.hpp>

static const char * statusTxt[] = { "Wifi down", "Connected", "Connection failed",
                                    "No matching SSID found", "Authenticatation failure" };
static const char * wifiLogId = "wifi";

VzPicoWifi::VzPicoWifi(uint numRetries) : sysRefTime(0), firstTime(true)
{
  retries = numRetries;
}
VzPicoWifi::~VzPicoWifi()
{
  this->disable();
}

/** ======================================================================
 * Enable and connect WiFi
 * To be done on demand
 * ====================================================================== */

bool VzPicoWifi::enable()
{
  // --------------------------------------------------------------
  print(log_info, "Enabling WiFi ...", wifiLogId);
  // --------------------------------------------------------------

  int rc;
  if(rc = cyw43_arch_init())
  {
    print(log_error, "WiFi init failed. Error %d.", wifiLogId, rc);
    return false;
  }
  cyw43_arch_enable_sta_mode();

  for(int i = 0; i < retries; i++)
  {
    // --------------------------------------------------------------
    print(log_debug, "Connecting WiFi %s (%d) ...", wifiLogId, wifiSSID, i);
    // --------------------------------------------------------------

    if(! (rc = cyw43_arch_wifi_connect_timeout_ms(wifiSSID, wifiPW, CYW43_AUTH_WPA2_AES_PSK, 30000)))
    {
      print(log_debug, "WiFi %s connected.", wifiLogId, wifiSSID);
      firstTime = false;
      return true;
    }
    sleep_ms(2000);
  }

  print(log_error, "Failed to connect WiFi '%s'. Error: %d.", wifiLogId, wifiSSID, rc);
  return false;
}

/** ======================================================================
 * Init WiFi - done only once at startup. Get NTP time.
 * No time yet, therefore no logger, just printf
 * ====================================================================== */

bool VzPicoWifi::init()
{
  if(this->enable())
  {
    // --------------------------------------------------------------
    printf("** Getting NTP time ...\n");
    // --------------------------------------------------------------

    Ntp ntp;
    time_t utc = ntp.queryTime();
    if(utc)
    {
      printf("** Got NTP UTC time %s", ctime(&utc));
      time(&sysRefTime); // Something like 1.1.1970 00:00:07, i.e. the sys is running 7secs
      sysRefTime = utc - sysRefTime;
      printf("** Sys boot time UTC %s", ctime(&sysRefTime));
    }
    return true;
  }
  return false;
}

/** ======================================================================
 * Disable WiFi to save power - to be done if not needed
 * ====================================================================== */

void VzPicoWifi::disable()
{
  print(log_info, "Disabling WiFi ...", wifiLogId);
  cyw43_arch_deinit();
  print(log_debug, "WiFi disabled.", wifiLogId);
}

time_t VzPicoWifi::getSysRefTime() { return sysRefTime; }

bool VzPicoWifi::isLinkUp()
{
  int linkStatus = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
  print(log_debug, "WiFi link status: %s (%d).", wifiLogId, statusTxt[linkStatus], linkStatus);
  return (linkStatus == CYW43_LINK_JOIN);
}
