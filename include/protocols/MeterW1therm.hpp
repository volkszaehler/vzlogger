/**
 * 1wire-support for therm sensors supported by linux modules w1-therm
 * functionality similar to 1wirevz
 *
 * @copyright Copyright (c) 2015 - 2023, the volkszaehler.org project
 * @license http://www.gnu.org/licenses/glp2.txt GNU Public License v2
 * @author Matthias Behr <mbehr (a) mcbehr.de>
 * */

#ifndef _meterw1therm_hpp_
#define _meterw1therm_hpp_

#include <protocols/Protocol.hpp>

class MeterW1therm : public vz::protocol::Protocol {
  public:
	// abstract hw interface access for testability:
	class W1HWif {
	  public:
		W1HWif(){};
		virtual ~W1HWif(){};

		virtual bool scanW1devices() = 0; // scan for w1 devices
		virtual const std::list<std::string> &
		W1devices() const = 0; // return current list of devices
		virtual bool readTemp(const std::string &dev, double &value) = 0;
	};

	class W1sysHWif : public W1HWif {
	  public:
		W1sysHWif(){};
		virtual ~W1sysHWif(){};

		virtual bool scanW1devices();
		virtual const std::list<std::string> &W1devices() const { return _devices; }
		virtual bool readTemp(const std::string &device, double &value);

	  protected:
		std::list<std::string> _devices;
	};

	MeterW1therm(const std::list<Option> &options,
				 W1HWif *hwif = 0); // class takes ownership of hwif. Can be 0 in this case will
									// default to W1syshwif
	virtual ~MeterW1therm();

	virtual int open();
	virtual int close();
	virtual ssize_t read(std::vector<Reading> &rds, size_t n);

  protected:
	W1HWif *_hwif;
};

#endif
