/**
 * Channel handling
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

#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <iostream>
#include <pthread.h>

#include "reading.h"
#include "buffer.h"
#include <threads.h>
#include <options.h>

class Channel {

public:
	typedef vz::shared_ptr<Channel> Ptr;

	Channel(const std::list<Option> &pOptions, const std::string api, const std::string pUuid, ReadingIdentifier::Ptr pIdentifier);
	virtual ~Channel();

  void start() {
    pthread_create(&_thread, NULL, &logging_thread, (void *) this);
  }
  
  void cancel() { pthread_cancel(_thread); }

  const char* name()                  { return _name.c_str(); }
  std::list<Option> &options()        { return _options; }

  ReadingIdentifier::Ptr identifier() { return _identifier; }
  const double tvtod() const          { return _last == NULL ? 0 : _last->tvtod(); }
  
  const char* uuid()                  { return _uuid.c_str(); }
  const std::string apiProtocol()     { return _apiProtocol; }

  void last(Reading *rd)              { _last = rd;}
  void push(const Reading &rd)        { _buffer->push(rd); }
	char *dump(char *dump, size_t len)  { return _buffer->dump(dump, len); }
  Buffer::Ptr buffer()                { return _buffer; }
  
  const size_t size() { return _buffer->size(); }  
  const size_t keep() { return _buffer->keep(); }  

  void notify() {
    _buffer->lock();
    pthread_cond_broadcast(&condition);
    _buffer->unlock();
  }
  void wait() {
    _buffer->lock();
    while(_buffer->size()==0) {
			_buffer->wait(&condition); /* sleep until new data has been read */
    }
    _buffer->unlock();
  }
  
  private:
	static int instances;
	int id;		       		  /**< only for internal usage & debugging */
  std::string _name;    /**< name of the channel */
  std::list<Option> _options;
  
	Buffer::Ptr _buffer;		/**< circular queue to buffer readings */
  
	ReadingIdentifier::Ptr _identifier;	/* channel identifier (OBIS, string) */
	Reading *_last;			/* most recent reading */

	pthread_cond_t condition;	 /**< pthread syncronization to notify logging thread and local webserver */
	pthread_t _thread;		     /**< pthread for asynchronus logging */

 	std::string _uuid;		   	 /**< unique identifier for middleware */
  std::string _apiProtocol;  /**< protocol of api to use for logging */
};

#endif /* _CHANNEL_H_ */
