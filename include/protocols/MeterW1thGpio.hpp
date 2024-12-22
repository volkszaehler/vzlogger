/**
 * w1therm sensor based on GPIO, based on this lib:
 * https://github.com/adamboardman/pico-onewire
 **/

#ifndef _meterw1therm_gpio_hpp_
#define _meterw1therm_gpio_hpp_

#include <protocols/Protocol.hpp>
#include <vector>

class One_wire;

class MeterW1thGpio : public vz::protocol::Protocol {
  public:
    MeterW1thGpio(const std::list<Option> &options);
    virtual ~MeterW1thGpio();

    virtual int open();
    virtual int close();
    virtual ssize_t read(std::vector<Reading> &rds, size_t n);

  private:
    u_int      w1pin;
    One_wire * one_wire;
    u_int      retryBadValues;
    u_int      retryDelay;
    int        goodValueLimits[2];

    std::vector<ReadingIdentifier::Ptr> ids;
};

#endif
