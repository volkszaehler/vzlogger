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

#define PICO_CYW43_ARCH_THREADSAFE_BACKGROUND 1
#include "pico/cyw43_arch.h"
#include "VzPicoWifi.h"
#include <Ntp.hpp>

static const char * statusTxt[] = { "Wifi down", "Connected", "Connection failed",
                                    "No matching SSID found", "Authentication failure" };
static const char * wifiLogId = "wifi";

VzPicoWifi::VzPicoWifi(const char * hn, uint numRetries, uint timeout) : sysRefTime(0), firstTime(true), numUsed(0), accTimeUp(0), accTimeDown(0), accTimeConnecting(0), initialized(false)
{
  retries = numRetries;
  connTimeout = timeout;
  lastChange = time(NULL);
  if(hn != NULL)
  {
    hostname = hn;
  }
}
VzPicoWifi::~VzPicoWifi()
{
  this->disable();
}

/** ======================================================================
 * Enable and connect WiFi
 * To be done on demand
 * ====================================================================== */

bool VzPicoWifi::enable(uint enableRetries)
{
  int rc;
  if(initialized)
  {
    int linkStatus = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    print(log_debug, "Not initializing WiFi - already done. Link status: %d", wifiLogId, linkStatus);
  }
  else
  { 
    // --------------------------------------------------------------
    print(log_info, "Enabling WiFi ...", wifiLogId);
    // --------------------------------------------------------------

    if(rc = cyw43_arch_init())
    {
      print(log_error, "WiFi init failed. Error %d.", wifiLogId, rc);
      return false;
    }
    cyw43_arch_enable_sta_mode();

    this->ledOn();

    cyw43_arch_lwip_begin();
    struct netif * n = &cyw43_state.netif[CYW43_ITF_STA];
    if(hostname.empty())
    {
      hostname = netif_get_hostname(n);
    }
    else
    {
      netif_set_hostname(n, hostname.c_str());
      netif_set_up(n);
    }
    cyw43_arch_lwip_end();
    initialized = true;
  }

  time_t now = time(NULL);
  accTimeDown += (now - lastChange);
  lastChange = now;

  for(int i = 0; i < (enableRetries > 0 ? enableRetries : retries); i++)
  {
    // --------------------------------------------------------------
    print(log_debug, "Connecting WiFi %s (%d) as '%s' ...", wifiLogId, wifiSSID, i, hostname.c_str());
    // --------------------------------------------------------------

    if(! (rc = cyw43_arch_wifi_connect_timeout_ms(wifiSSID, wifiPW, CYW43_AUTH_WPA2_AES_PSK, connTimeout)))
    {
      int32_t rssi;
      uint8_t mac[6];
      
      cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac);
      cyw43_wifi_get_rssi(&cyw43_state, &rssi);
      cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);

      print(log_info, "WiFi '%s' connected. Signal: %d, MAC: %02x:%02x:%02x:%02x:%02x:%02x, IP: %s", wifiLogId, wifiSSID, rssi,
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ipaddr_ntoa(&cyw43_state.netif[CYW43_ITF_STA].ip_addr));
      firstTime = false;

      now = time(NULL);
      accTimeConnecting += (now - lastChange);
      lastChange = now;
      numUsed++;

      return true;
    }
    sleep_ms(1000);
    this->ledOn(10);
  }

  print(log_error, "Failed to connect WiFi '%s'. Error: %d.", wifiLogId, wifiSSID, rc);
  this->disable();
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
  if(! initialized)
  {
    return;
  }

  time_t now = time(NULL);
  accTimeUp += (now - lastChange);
  lastChange = now;

  print(log_info, "Disabling WiFi ...", wifiLogId);
  cyw43_arch_deinit();
  print(log_debug, "WiFi disabled.", wifiLogId);
  initialized = false;
}

time_t VzPicoWifi::getSysRefTime() { return sysRefTime; }

bool VzPicoWifi::isConnected()
{
  int linkStatus = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
  print(log_debug, "WiFi link status: %s (%d).", wifiLogId, statusTxt[linkStatus], linkStatus);
  if(linkStatus < 0)
  {
    print(log_error, "WiFi link status: %d.", wifiLogId, linkStatus);
  }
  return (linkStatus == CYW43_LINK_JOIN);
}

void VzPicoWifi::printStatistics(log_level_t logLevel)
{
  // Power consumption:
  //  Connecting ~60mA
  //  Up ~40mA
  //  Down ~20mA (with full CPU speed)
  print(logLevel, "WiFi statistics: NumUsed: %d, Time connecting: %ds, up: %ds, down: %ds", wifiLogId,
        numUsed, accTimeConnecting, accTimeUp, accTimeDown);
}

void VzPicoWifi::ledOn(uint msecs)
{
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
  if(msecs > 0)
  {
    // Otherwise leave it on
    sleep_ms(msecs);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
  }
}

