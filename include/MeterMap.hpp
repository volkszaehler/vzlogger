/***********************************************************************/
/** @file MeterMap.hpp
 *
 *  @author Kai Krueger
 *  @date   2012-03-13
 *  @email  kai.krueger@itwm.fraunhofer.de
 *
 * (C) Fraunhofer ITWM
 **/
/*---------------------------------------------------------------------*/

/**
 * Protocol interface
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

#ifndef _MeterMap_hpp_
#define _MeterMap_hpp_
#include <pthread.h>
#include <vector>

#include <Channel.hpp>
#include <Meter.hpp>
#include <Options.hpp>
#include <common.h>

/**
	 The MeterMap is intend to keep the list of all configured channel for a given meter.
*/
class MeterMap {
  public:
	typedef vz::shared_ptr<MeterMap> Ptr;
	typedef std::vector<Channel::Ptr>::iterator iterator;
	typedef std::vector<Channel::Ptr>::const_iterator const_iterator;

	MeterMap(const std::list<Option> &options) : _meter(new Meter(options)) {
		_thread_running = false;
	}
	MeterMap(Meter *m) : _meter(m), _thread_running(false){};
	~MeterMap(){};
	Meter::Ptr meter() { return _meter; }

	/**
		 If the meter is enabled, start the meter and all its channels.
	*/
	void start();

	/**
	 * cancel all channels for this meter.
	 */
	void cancel();

	/**
	 * send device-registration for each channel
	 */
	void registration();

	/**
	 *  Accessor to the channel list
	 */
	inline void push_back(Channel::Ptr channel) { _channels.push_back(channel); }
	inline iterator begin() { return _channels.begin(); }
	inline iterator end() { return _channels.end(); }
	inline size_t size() { return _channels.size(); }

	bool running() const { return _thread_running; }

  private:
	Meter::Ptr _meter;
	std::vector<Channel::Ptr> _channels;

	bool _thread_running; // flag if thread is started
	pthread_t _thread;    // Thread data for meter (reading)
};

/**
	 This container is intend to keep the list of all configured meters.
*/
class MapContainer {
  public:
	typedef vz::shared_ptr<MapContainer> Ptr;
	typedef std::vector<MeterMap>::iterator iterator;
	typedef std::vector<MeterMap>::const_iterator const_iterator;

	MapContainer(){};
	~MapContainer(){};

	/**
	 *  Accessor to the MeterMap (meter and its channels) list
	 */
	inline void push_back(const MeterMap &map) { _mappings.push_back(map); }
	inline iterator begin() { return _mappings.begin(); }
	inline iterator end() { return _mappings.end(); }
	inline size_t size() const { return _mappings.size(); }

  private:
	std::vector<MeterMap> _mappings;
};
#endif /* _MeterMap_hpp_ */
