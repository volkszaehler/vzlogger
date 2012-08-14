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

#include <Reading.hpp>
#include <Options.hpp>
#include <Channel.hpp>
#include <shared_ptr.hpp>
#include <meter_protocol.hpp>
#include <protocols/Protocol.hpp>

class Meter {

public:
	typedef vz::shared_ptr<Meter> Ptr;

	Meter(std::list<Option> options);
//	Meter(const Meter *mtr);
	virtual ~Meter();

	void open();
	int close();
	size_t read(std::vector<Reading> &rds, size_t n);

// setter
	void interval(const int i) { _interval = i; }

// getter
	const char *name() const               { return _name.c_str(); }
	const bool isEnabled() const           { return _enable; }

	const meter_protocol_t protocolId() const { return _protocol_id; } 
	vz::protocol::Protocol::Ptr protocol() { return _protocol; }

	ReadingIdentifier::Ptr identifier()    { return _identifier; }

	const int  interval() const            { return _interval; }

private:
	static int instances;                   /**< meter instance id (increasing counter) */
	bool _thread_running;   								/**< flag if thread is started */

	int id;                                 /**< meter id */
	std::string _name;                      /**< meter name */
	bool _enable;                           /**< true if meter is disabled (default) */

	meter_protocol_t _protocol_id;          /**< meter protocol id */
	vz::protocol::Protocol::Ptr _protocol;  /**< meter protocol */

	ReadingIdentifier::Ptr _identifier;


	int _interval;

	std::vector<Channel> channels;          /**< channel for logging */
};

typedef struct {
	const meter_protocol_t id;
	const char *name;		/* short identifier for protocol */
	const char *desc;		/* more detailed description */
	const size_t max_readings;	/* how many readings can be read with 1 call */
	const bool periodic;		/* does this meter has be triggered periodically? */
} meter_details_t;

/**
 * Get list of available meter types
 */

int meter_lookup_protocol(const char* name, meter_protocol_t *protocol);
const meter_details_t * meter_get_protocols();
const meter_details_t * meter_get_details(meter_protocol_t protocol);

#endif /* _METER_H_ */
