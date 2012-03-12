/**
 * Meter interface
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011, The volkszaehler.org project
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

#ifndef _METER_H_
#define _METER_H_
#include <list>
#include <vector>

//#include "list.h"
#include <reading.h>
#include <options.h>
#include <channel.h>
#include <shared_ptr.hpp>
#include <meter_protocol.hpp>
#include <protocols/protocol.hpp>

class Meter {

public:
  typedef vz::shared_ptr<Meter> Ptr;
  
	Meter(std::list<Option> options);
	Meter(const Meter *mtr);
	virtual ~Meter();

  void init(std::list<Option> options);
  void open();
  int close();
  size_t read(std::vector<Reading> &rds, size_t n);

  void interval(const int i) { _interval = i; }

  vz::protocol::Protocol::Ptr protocol() { return _protocol; }
  ReadingIdentifier::Ptr identifier() { return _identifier; }
  const meter_protocol_t protocolId() const { return _protocol_id; } 
  const bool isEnabled() const { return _enable; }
  const char *name() const { return _name.c_str(); }
  const int  interval() const { return _interval; }

protected:
	//Meter(std::list<Option> options);

  private:
  vz::protocol::Protocol::Ptr _protocol;
  ReadingIdentifier::Ptr _identifier;
  meter_protocol_t _protocol_id;
	int id;
  std::string _name;
  bool _enable;
  
	static int instances;

	int _interval;

	std::vector<Channel> channels;

	pthread_t thread;
};

typedef struct {
	const meter_protocol_t id;
	const char *name;		/* short identifier for protocol */
	const char *desc;		/* more detailed description */
	const size_t max_readings;	/* how many readings can be read with 1 call */
	const int periodic:1;		/* does this meter has be triggered periodically? */

  //const Meter _meter;
  
} meter_details_t;

/**
 * Get list of available meter types
 */

int meter_lookup_protocol(const char* name, meter_protocol_t *protocol);
const meter_details_t * meter_get_protocols();
const meter_details_t * meter_get_details(meter_protocol_t protocol);

#endif /* _METER_H_ */
