
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#define PICO_CYW43_ARCH_THREADSAFE_BACKGROUND 1

#include "lwip/netif.h"
#include "lwip/init.h"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "hardware/watchdog.h"

#include <Config_Options.hpp>
#include <Meter.hpp>
#include <malloc.h>

#include "VzPicoWifi.h"

// Shutdown WiFi is at least 60s down, else not worth it
// TODO: Should be configurable ...
static const uint wifiStopTime = 60;

// --------------------------------------------------------------
// Global vars referenced elsewhere
// --------------------------------------------------------------

time_t         sysRefTime = 0;
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

  printf("--------------------------------------------------------------\n");
  printf("** vzlogger %s/%s (LwIP %s) starting up ...\n", PACKAGE, VERSION, LWIP_VERSION_STRING);
  printf("** Connecting WiFi ...\n");
  printf("--------------------------------------------------------------\n");

  options.verbosity(5); // INFO at startup, will be overwritte when parsing config

  VzPicoWifi wifi;
  if(! wifi.init())
  {
    fprintf(stderr, "*** ERROR: Wi-Fi connect failed");
    return EXIT_FAILURE;
  }
  sysRefTime = wifi.getSysRefTime();
  if(! sysRefTime)
  {
    fprintf(stderr, "*** ERROR: Failed to get NTP time.");
    return EXIT_FAILURE;
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
    bool keepWifi = false;

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
          if(wifi.isLinkUp() || wifi.enable())
          {
            it->sendData();
            keepWifi = true;
          }
        }
        else if(it->isBusy())
        {
          keepWifi = true;
        }
      }
    }
    catch (std::exception &e)
    {
      print(log_alert, "Reading meter failed: %s", "", e.what());
      // sleep_ms(500); // To see what happened via USB
      // exit(1);
      // We don't exit, just go on, try to recover whatever possibe
    }

    if((nextDue > 0) && ((cycle % 10) == 0))
    {
      print(log_debug, "Cycle %d: All meters and pending I/O processed. Next due: %ds. Napping ...", "", cycle, nextDue);

      struct mallinfo m = mallinfo();
      extern char __StackLimit, __bss_end__;
      print(log_info, "Cycle %d, MEM: Used: %ld, Free: %ld", "", cycle, m.uordblks, (&__StackLimit  - &__bss_end__) - m.uordblks);
    }
    cycle++;

    // Sleep a while ... TODO config, or even calculate from config
    sleep_ms(1000);

    // --------------------------------------------------------------
    // Blink the LED to see something is happening
    // Cannot use onboard LED as this is preventing from disabling WiFi if not needed (for saving energy)
    // So, do that only if currently WiFi is on
    // --------------------------------------------------------------

    if(wifi.isLinkUp())
    {
      cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
      sleep_ms(10);
      cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

      if((! keepWifi) && (nextDue > wifiStopTime))
      {
        wifi.disable();
      }
    }
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

  char prefix[24];
  if(sysRefTime > 0)
  {
    struct timeval now;
    gettimeofday(&now, NULL);
    now.tv_sec += sysRefTime + tzOffset;
    struct tm tm;
    struct tm * timeinfo = localtime_r(&now.tv_sec, &tm);

    /* format timestamp */
    size_t pos = strftime(prefix, 18, "[%b %d %H:%M:%S]", timeinfo);

    /* format section */
    if (id)
    {
      snprintf(prefix + pos, 8, "[%s]", (char *)id);
    }
  }
  else
  {
    strcpy(prefix, "** ");
  }

  va_list args;
  va_start(args, id);
  printf((sysRefTime > 0 ? "%-24s" : "%s"), prefix);
  vprintf(format, args);
  printf("\n");
  va_end(args);
}

// --------------------------------------------------------------
// Will be called in Out-of-memory situation. Do a HW reset (not 100% reliable)
// --------------------------------------------------------------

extern "C" void vzlogger_panic(const char * fmt, ...)
{
  extern char __StackLimit, __bss_end__;
  printf("PANIC: %s stacklimit: %p, BSS end: %p, heap: %d, stack: %d\n", fmt, &__StackLimit, &__bss_end__, PICO_HEAP_SIZE, PICO_STACK_SIZE);
  watchdog_enable(1, 1);
}
