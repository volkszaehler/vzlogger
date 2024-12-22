/**
 * w1therm sensor based on GPIO, based on this lib:
 * https://github.com/adamboardman/pico-onewire
 **/

#include "Options.hpp"
#include "protocols/MeterW1thGpio.hpp"
#include "one_wire.h"

MeterW1thGpio::MeterW1thGpio(const std::list<Option> &options) : Protocol("w1tGpio"), one_wire(NULL)
{
  OptionList optlist;
  const char * optName;
  try
  {
    optName = "w1pin"; w1pin = optlist.lookup_int(options, optName);
  }
  catch (vz::VZException &e)
  {
    print(log_alert, "Missing '%s' or invalid type", name().c_str(), optName);
    throw;
  }
  print(log_debug, "Created MeterW1thGpio (GPIO %d)", "", w1pin);

  // If the measured value is outside of these bounds, retry, but max <n> times
  // If still no good value, skip
  goodValueLimits[0] = -20;
  goodValueLimits[1] = 80;
  retryBadValues = 5;
  retryDelay = 5; // in ms
}

MeterW1thGpio::~MeterW1thGpio()
{
  print(log_debug, "Deleting oneWire object %p", "", one_wire);
  delete one_wire;
}

int MeterW1thGpio::open()
{
  print(log_debug, "Opening MeterW1thGpio (GPIO %d) ...", "", w1pin);
  one_wire = new One_wire(w1pin);
  print(log_debug, "Created oneWire object %p, Initializing ...", "", one_wire);
  one_wire->init();
  print(log_debug, "Initialized oneWire object %p.", "", one_wire);
  sleep_ms(1);
  return SUCCESS;
}

int MeterW1thGpio::close() { return SUCCESS; }

ssize_t MeterW1thGpio::read(std::vector<Reading> &rds, size_t n)
{
  ssize_t ret = 0;

  print(log_debug, "Reading oneWire object %p ...", "", one_wire);
  int count = one_wire->find_and_count_devices_on_bus();
  print(log_debug, "Have %d oneWire devices.", "", count);
  rom_address_t null_address{};
  one_wire->convert_temperature(null_address, true, true);
  for (int i = 0; i < count && i < n; i++)
  {
    auto address = One_wire::get_address(i);
    char str[32];
    sprintf(str, "%016llX", One_wire::to_uint64(address));

    u_int idIdx = 0;
    StringIdentifier strID(str);
    for(; idIdx < ids.size(); idIdx++)
    {
      if(*(ids[idIdx]) == strID)
      {
        print(log_debug, "Already have ID[%d]: %s", "", idIdx, str);
        break;
      }
    }
    if(idIdx == ids.size())
    {
      print(log_debug, "Adding ID[%d]: %s", "", idIdx, str);
      ids.push_back(ReadingIdentifier::Ptr(new StringIdentifier(str)));
    }

    print(log_debug, "Reading oneWire device[%d] '%s'.", "", i, str);

    for(int j = 0; j < retryBadValues; j++)
    {
      double value = one_wire->temperature(address);
      print(log_finest, "reading w1 device %s returned %f", name().c_str(), str, value);

      // If the measured value is outside of these bounds, retry, but max <n> times
      // If still no good value, skip

      if(value >= goodValueLimits[0] && value <= goodValueLimits[1])
      {
        rds[ret].identifier(ids[idIdx]);
        rds[ret].time();
        rds[ret].value(value);
        ++ret;
        break;
      }

      sleep_ms(retryDelay);
    }
  }

  return ret;
}
