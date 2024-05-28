
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#define PICO_CYW43_ARCH_THREADSAFE_BACKGROUND 1

#include "lwip/netif.h"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "hardware/watchdog.h"

#include <Config_Options.hpp>
#include <Meter.hpp>
#include <Ntp.hpp>

// --------------------------------------------------------------
// Pico has no filesystem, hence no config file to read, hence inline config:
// --------------------------------------------------------------

static const char * inlineConfig =
"{ 'verbosity': 10, \
   'retry': 30, \
   'meters': \
   [ \
     { \
       'enabled': true, \
       'skip': false, \
       'interval': 120, \
       'protocol': 'onboardTemp', \
       'unit': 'C', \
       'channels': \
       [ \
         { \
           'uuid': '3a145ba0-db39-11ee-a6a8-57d706d8e278', \
           'api': 'volkszaehler', \
           'middleware': 'http://vz-server:8000/middleware.php' \
         } \
       ] \
     }, \
     { \
       'enabled': true, \
       'skip': false, \
       'interval': 10, \
       'protocol': 'sct013', \
       'adcnum': 0, \
       'scttype': 30, \
       'delay': 10, \
       'numsamples': 1480, \
       'channels': \
       [ \
         { \
           'uuid': 'f3ef9b70-de3b-11ee-83b5-73042e2a7e09', \
           'api': 'volkszaehler', \
           'middleware': 'http://vz-server:8000/middleware.php', \
           'identifier': 'Power'\
         } \
       ] \
     } \
   ] }";

/* Not needed anymore - redundant with Power, just fills up DB
         { \
           'uuid': '2e2a8c90-dd66-11ee-9621-0d0747854c29', \
           'api': 'volkszaehler', \
           'middleware': 'http://vz-server:8000/middleware.php', \
           'identifier': 'Current' \
         }, \
*/

static const char * wifiSSID = TODO "";
static const char * wifiPW   = TODO "";

static const uint tzOffset = 0; // 3600; // 1h ahead of UTC

// --------------------------------------------------------------
// Global vars referenced elsewhere
// --------------------------------------------------------------

time_t         sysRefTime;
Config_Options options;    // global application options

// --------------------------------------------------------------
// Main function 
// --------------------------------------------------------------

int main()
{
  stdio_init_all();

  // Wait a bit at beginning to see stdout on USB. Say:
  //  sudo screen -L /dev/ttyACM0
  sleep_ms(5000);

  // --------------------------------------------------------------
  printf("** Starting ... init WIFi ...\n");
  // --------------------------------------------------------------

  if (cyw43_arch_init())
  {
    fprintf(stderr, "ERROR: Wi-Fi init failed");
    return EXIT_FAILURE;
  }
  cyw43_arch_enable_sta_mode();

  // --------------------------------------------------------------
  printf("** Connecting WIFi %s ...\n", wifiSSID);
  // --------------------------------------------------------------

  bool connected = false;
  for(int i = 0; i < 20; i++)
  {
    if(! cyw43_arch_wifi_connect_timeout_ms(wifiSSID, wifiPW, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
      connected = true;
      break;
    }
    printf("Failed to connect WiFi '%s'\n", wifiSSID);
    sleep_ms(2000);
  }
  if(! connected)
  {
    fprintf(stderr, "ERROR: Wi-Fi connect failed");
    return EXIT_FAILURE;
  }

  // --------------------------------------------------------------
  printf("** Getting NTP time ...\n");
  // --------------------------------------------------------------

  {
    // In block, so ntp gets deconstructed, not needed anymore
    Ntp ntp;
    time_t utc = ntp.queryTime();
    printf("** Got NTP UTC time %s", ctime(&utc));
    utc += tzOffset; // Add TZ offset
    time(&sysRefTime); // Something like 1.1.1970 00:00:07
    sysRefTime = utc - sysRefTime;
    printf("** Sys boot time    %s", ctime(&sysRefTime));
  }

  // --------------------------------------------------------------
  printf("** Parsing config and creating meters ...\n");
  // --------------------------------------------------------------

  MapContainer mappings;     // mapping between meters and channels
  try
  {
    options.config_parse(mappings, inlineConfig);
  }
  catch (std::exception &e)
  {
    print(log_alert, "Failed to parse configuration due to: %s", NULL, e.what());
    return EXIT_FAILURE;
  }
  if (mappings.size() <= 0)
  {
    print(log_alert, "No meters found - quitting!", NULL);
    return EXIT_FAILURE;
  }

  // --------------------------------------------------------------
  print(log_debug, "ADC init", "");
  // --------------------------------------------------------------

  adc_init();

  // --------------------------------------------------------------
  print(log_debug, "===> Start meters", "");
  // --------------------------------------------------------------

  try
  {
    // open connection meters
    for (MapContainer::iterator it = mappings.begin(); it != mappings.end(); it++)
    {
      it->start();
    }
  }
  catch (std::exception &e)
  {
    print(log_alert, "Startup failed: %s", "", e.what());
    exit(1);
  }
  print(log_debug, "Startup done.", "");

  // --------------------------------------------------------------
  // Main loop
  // --------------------------------------------------------------

  uint cycle = 0;
  while(true)
  {
    // --------------------------------------------------------------
    // Blink the LED ... TODO maybe disable this later ...
    // --------------------------------------------------------------

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(10);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    // --------------------------------------------------------------
    // Check the meters ...
    // --------------------------------------------------------------

    int nextDue = 0;
    try
    {
      // read meters
      for (MapContainer::iterator it = mappings.begin(); it != mappings.end(); it++)
      {
        int due = it->isDueIn();
        if(due <= 0)
        {
          it->read();
        }
        else if(nextDue == 0 || due < nextDue)
        {
          nextDue = due;
        }

        if(it->readyToSend())
        {
          it->sendData();
        }
      }
    }
    catch (std::exception &e)
    {
      print(log_alert, "Reading meter failed: %s", "", e.what());
//      sleep_ms(500); // To see what happened via USB
//      exit(1);
    }

    if((nextDue > 0) && ((cycle % 10) == 0))
    {
      print(log_debug, "Cycle %d: All meters and pending I/O processed. Next due: %ds. Napping ...", "", cycle, nextDue);
    }
    cycle++;

    // Sleep a while ... TODO config, or even calculate from config
    sleep_ms(1000);
  }

  return EXIT_SUCCESS;
}

/** --------------------------------------------------------------
 * Print error/debug/info messages to stdout and/or logfile
 *
 * @param id could be NULL for general messages
 * @todo integrate into syslog
 * -------------------------------------------------------------- */

void print(log_level_t level, const char *format, const char *id, ...)
{
  if (level > options.verbosity())
  {
    return; /* skip message if its under the verbosity level */
  }

  struct timeval now;
  gettimeofday(&now, NULL);
  now.tv_sec += sysRefTime;
  struct tm tm;
  struct tm * timeinfo = localtime_r(&now.tv_sec, &tm);

  /* format timestamp */
  char prefix[24];
  size_t pos = strftime(prefix, 18, "[%b %d %H:%M:%S]", timeinfo);

  /* format section */
  if (id)
  {
    snprintf(prefix + pos, 8, "[%s]", (char *)id);
  }

  va_list args;
  va_start(args, id);
  printf("%-24s", prefix);
  vprintf(format, args);
  printf("\n");
  va_end(args);
}

extern "C" void vzlogger_panic(const char * fmt, ...)
{
  printf("PANIC\n");
  watchdog_enable(1, 1);
}
