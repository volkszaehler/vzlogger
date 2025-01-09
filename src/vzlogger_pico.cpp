
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#include "lwip/init.h"
#include "pico/stdlib.h"
// #include "pico/cyw43_arch.h"
#include "hardware/watchdog.h"

#include <Config_Options.hpp>
#include <Meter.hpp>

#include "VzPicoWifi.h"
#include "VzPicoSys.h"

// No major benefit from being user-configurable
static const uint mainLoopSleep = 1000;    // in ms

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
  // -----------------------------------------
  // Must do this at very beginning - in any case before "stdio_init_all", otherwise USB stdio stuck
  // -----------------------------------------
  VzPicoSys vzPicoSys;
  int rc = vzPicoSys.init();
  if(rc < 0)
  {
    fprintf(stderr, "*** ERROR: Init Vsys failed: %d\n", rc);
  }

  stdio_init_all();

  // Wait a bit at beginning to see stdout on USB. Say:
  //  sudo screen -L /dev/ttyACM0
  sleep_ms(5000);

  printf("--------------------------------------------------------------\n");
  printf("** vzlogger %s/%s (LwIP %s) starting up ...\n", PACKAGE, VERSION, LWIP_VERSION_STRING);
  printf("** Connecting WiFi ...\n");
  printf("--------------------------------------------------------------\n");

  options.verbosity(5); // INFO at startup, will be overwritten when parsing config

  VzPicoWifi wifi(myHostname);
  if(! wifi.init())
  {
    fprintf(stderr, "*** ERROR: Wi-Fi connect failed.\n");
    return EXIT_FAILURE;
  }
  sysRefTime = wifi.getSysRefTime();
  if(! sysRefTime)
  {
    fprintf(stderr, "*** ERROR: Failed to get NTP time.\n");
    return EXIT_FAILURE;
  }

  // --------------------------------------------------------------
  print(log_debug, "Parsing config and creating meters ...", "");
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

  print(log_info, "Default clock speed: %dMHz.", "", vzPicoSys.getCurrentClockSpeed());

  time_t sendDataComplete = time(NULL);
  uint cycle = 0;
  while(true)
  {
    // --------------------------------------------------------------
    // Check the meters ...
    // --------------------------------------------------------------

    int  nextDue = 0;
    bool keepWifi = false;
    bool clockSpeedIsDefault = vzPicoSys.isClockSpeedDefault();

    try
    {
      // --------------------------------------------------------------
      // Read meters if due ...
      // --------------------------------------------------------------

      for (MapContainer::iterator it = mappings.begin(); it != mappings.end(); it++)
      {
        int due = it->isDueIn();
        if(due <= 0)
        {
          // Some meters depend on default clock speed - so enable
          if(! clockSpeedIsDefault)
          {
            clockSpeedIsDefault = vzPicoSys.setCpuSpeedDefault();
          }
          it->read();
        }
        else if(nextDue == 0 || due < nextDue)
        {
          nextDue = due;
        }

        // --------------------------------------------------------------
        // If there is data to be sent + last sending is longer ago than "sendDataInterval"
        // --------------------------------------------------------------

        if(it->readyToSend() && ((time(NULL) - sendDataComplete) > sendDataInterval))
        {
          // Also, WiFi does not work right on lower speeds
          if(! clockSpeedIsDefault)
          {
            clockSpeedIsDefault = vzPicoSys.setCpuSpeedDefault();
          }

          bool wifiConnected = wifi.isConnected();
          if(! wifiConnected)
          {
            wifiConnected = wifi.enable();
          }

          if(wifiConnected)
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
      // We don't exit, just go on, try to recover whatever possibe
      print(log_alert, "Reading meter failed: %s", "", e.what());
    }

    if((nextDue > 0) && ((cycle % 10) == 0))
    {
      print(log_info, "Cycle %d: All meters and pending I/O processed. Next due: %ds. Napping ...", "", cycle, nextDue);

      // TODO: Make these metrics available as a meter (?)
      print(log_debug, "Cycle %d, MEM: Used: %ld, Free: %ld", "", cycle, vzPicoSys.getMemUsed(), vzPicoSys.getMemFree());
    }

    // --------------------------------------------------------------
    // Blink the LED to see something is happening
    // Cannot use onboard LED as this is preventing from disabling WiFi if not needed (for saving energy)
    // So, do that only if currently WiFi is on
    // --------------------------------------------------------------

    // In low-power mode, do not even query WiFi
    if(clockSpeedIsDefault && wifi.isConnected())
    {
      // Blink LED for 10ms (depends on WiFi, so only if WiFi is on) saying: "WiFi is on"
      wifi.ledOn(10);

      if((cycle % 10) == 0)
      {
        // Seems to depend on WiFi chip - so do this only if WiFi is on
        // TODO: Make these metrics available as a meter (?)
        print(log_info, "Power Source: %s Voltage: %.2f", "", (vzPicoSys.isOnBattery() ? "Battery" : "USB"), vzPicoSys.getVoltage());
      }

      // Shutdown Wifi if either it's worth (nextDue > wifiStopTime) or data sending 
      // is scheduled separately
      if((! keepWifi) && ((nextDue > wifiStopTime) || (sendDataInterval > 0)))
      {
        wifi.disable();
        sendDataComplete = time(NULL);
      }
      // TODO: Make these metrics available as a meter (?)
      wifi.printStatistics(log_info);
    }

    // Reduce clock speed after everything WiFi or peripheral is done
    if(clockSpeedIsDefault && ! wifi.isConnected())
    {
      vzPicoSys.setCpuSpeedLow(lowCPUfactor);
      // TODO: Make these metrics available as a meter (?)
      vzPicoSys.printStatistics(log_info);
      for (MapContainer::iterator it = mappings.begin(); it != mappings.end(); it++)
      {
        it->printStatistics(log_info);
      }
    }

    // Sleep a while ...
    sleep_ms(mainLoopSleep);

    cycle++;
  } // while

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
      snprintf(prefix + pos, 7, "[%s]", (char *)id);
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

