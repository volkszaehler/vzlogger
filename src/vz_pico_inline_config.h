
#ifndef VZ_INLINE_CONFIG
#define VZ_INLINE_CONFIG

# ifdef VZ_PICO_CUSTOM_CONFIG
#  include VZ_PICO_CUSTOM_CONFIG
# else // VZ_PICO_CUSTOM_CONFIG

// Some examples:

#define VZ_SERVER_URL "http://vz-server:8000/middleware.php"

static const uint tzOffset   = 3600; // 1h ahead of UTC
static const char * wifiSSID = "TODO";
static const char * wifiPW   = "TODO";
static const char * myHostname   = "TODO";

static const uint wifiStopTime = 60;       // Shutdown WiFi is at least 60s down from now on, else not worth it
static const uint lowCPUfactor = 20;       // Energy-saving low CPU speed, results in 6Mhz (default is 125MHz)
static const uint sendDataInterval = 300;  // Send data not more often than 5min, immediately if 0

// log_alert = 0, log_error = 1, log_warning = 3, log_info = 5, log_debug = 10, log_finest = 15
static const char * inlineConfig =
"{ 'verbosity': 15, \
   'retry': 30, \
   'meters': \
   [ \
     { \
       'enabled': true, \
       'skip': false, \
       'interval': 10, \
       'protocol': 'emonlib', \
       'adcCurrent': 0, \
       'adcVoltage': 1, \
       'currentCalibration' : 30, \
       'voltageCalibration' : 247.0, \
       'phaseCalibration' : 35.0, \
       'delay': 100, \
       'numSamples': 20, \
       'channels': \
       [ \
         { \
           'uuid': 'f3ef9b70-de3b-11ee-83b5-73042e2a7e09', \
           'api': 'volkszaehler', \
           'middleware': '" VZ_SERVER_URL "', \
           'identifier': 'RealPower' \
         } \
       ] \
     } \
    ,{ \
       'enabled': true, \
       'skip': false, \
       'interval': 10, \
       'protocol': 'emonlib', \
       'adcCurrent': 2, \
       'adcVoltage': 1, \
       'currentCalibration' : 30, \
       'voltageCalibration' : 247.0, \
       'phaseCalibration' : 132.0, \
       'delay': 100, \
       'numSamples': 20, \
       'channels': \
       [ \
         { \
           'uuid': 'a8d6bba0-6462-11ef-aea1-3b1a1ab3364e', \
           'api': 'volkszaehler', \
           'middleware': '" VZ_SERVER_URL "', \
           'identifier': 'RealPower' \
         } \
       ] \
     } \
   ] }";

/* Other examples:

---------------------------------------------------
Additional metrics from EmonLib ... normally not needed ...
---------------------------------------------------

        ,{ \
           'uuid': '560ff4e0-2d94-11ef-9a04-7f5c06e34262', \
           'api': 'volkszaehler', \
           'middleware': '" VZ_SERVER_URL "', \
           'identifier': 'Voltage'\
         }
        ,{ \
           'uuid': '2e2a8c90-dd66-11ee-9621-0d0747854c29', \
           'api': 'volkszaehler', \
           'middleware': '" VZ_SERVER_URL "', \
           'identifier': 'Current' \
         } \

---------------------------------------------------
Onboard temperature:
---------------------------------------------------

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
           'middleware': '" VZ_SERVER_URL "' \
         } \
       ] \
     }

---------------------------------------------------
HC-SR04 distance sensor + 2x DS18B20 w2 therm sensor (GPIO-based):
---------------------------------------------------

   'meters': \
   [ \
     { \
       'enabled': true, \
       'skip': false, \
       'interval': 60, \
       'protocol': 'w1tGpio', \
       'w1pin': 26, \
       'channels': \
       [ \
         { \
           'uuid': '815e8820-bd87-11ef-992f-017e1a2d8ec8', \
           'api': 'volkszaehler', \
           'middleware': '" VZ_SERVER_URL "', \
           'identifier': '2880A6860000008B'\
         } \
        ,{ \
           'uuid': '9757f240-bd87-11ef-a7dd-57cb8572778f', \
           'api': 'volkszaehler', \
           'middleware': '" VZ_SERVER_URL "', \
           'identifier': '284208830000006A'\
         }"
      "] \
     } \
    ,{ \
       'enabled': true, \
       'skip': false, \
       'interval': 10, \
       'protocol': 'hcsr04', \
       'trigger': 0, \
       'pioInst': 0, \
       'channels': \
       [ \
         { \
           'uuid': '600884e0-bd87-11ef-95ef-d9dde5b5e151', \
           'api': 'volkszaehler', \
           'middleware': '" VZ_SERVER_URL "', \
           'identifier': 'Distance'\
         }"
      "] \
     } \
   ] }";
*/

# endif // VZ_PICO_CUSTOM_CONFIG
#endif // VZ_INLINE_CONFIG

