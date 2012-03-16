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

#ifndef _MeterMap_hpp_
#define _MeterMap_hpp_
#include <pthread.h>
#include <vector>

#include <common.h>
#include <options.h>
#include <meter.h>
#include <channel.h>

/**
   The MeterMap is intend to keep the list of all configured channel for a given meter.
 */
class MeterMap {
  public:
  typedef vz::shared_ptr<MeterMap> Ptr;
  typedef std::vector<Channel>::iterator iterator;
  typedef std::vector<Channel>::const_iterator const_iterator;

  MeterMap(std::list<Option> options) : _meter(new Meter(options)){}
  ~MeterMap() {};
  Meter::Ptr meter() { return _meter; }

  void stop() {}
  
  /**
     If the meter is enabled, start the meter and all its channels.
   */
  void start() {
    if(_meter->isEnabled()) {
      _meter->open();
      print(log_debug, "meter is opened. Start reader.", _meter->name());
      pthread_create(&_thread, NULL, &reading_thread, (void *) this);
      print(log_debug, "meter is opened. Start channels.", _meter->name());
      for(iterator it = _channels.begin(); it!=_channels.end(); it++) {
        it->start();
      }
    } else {
      print(log_debug, "Skipping disabled meter.", _meter->name());
    }
    
  }
  
  /**
   * cancel all channels for this meter.
   */
  void cancel() {
    pthread_cancel(_thread);
    for(iterator it = _channels.begin(); it!=_channels.end(); it++) {
      it->cancel();
    }
  }
  
  /** 
   *  Accessor to the channel list
   */
  inline void push_back(Channel channel) { _channels.push_back(channel); }
  inline iterator begin()  { return _channels.begin(); }
  inline iterator end()    { return _channels.end(); }
  inline size_t size()     { return _channels.size(); }

  private:
  Meter::Ptr _meter;
	std::vector<Channel> _channels;

	pthread_t _thread;
};

/**
   This container is intend to keep the list of all configured meters. 
 */
class MapContainer {
  public:
  typedef vz::shared_ptr<MapContainer> Ptr;
  typedef std::vector<MeterMap>::iterator iterator;
  typedef std::vector<MeterMap>::const_iterator const_iterator;

  MapContainer() {};
  ~MapContainer() {};

  void quit(int sig) {
    print(log_info, "Closing connections to terminate", (char*)0);

    for(iterator it = _mappings.begin(); it!=_mappings.end(); it++) {
      it->cancel();
    }
  }
  
  /** 
   *  Accessor to the MeterMap (meter and its channels) list
   */
  inline void push_back(const MeterMap &map) { _mappings.push_back(map); } 
  inline iterator begin()  { return _mappings.begin(); }
  inline iterator end()    { return _mappings.end(); }
  inline const size_t size() const { return _mappings.size(); }
  
  private:
  std::vector<MeterMap> _mappings;
  
};
#endif /* _MeterMap_hpp_ */
