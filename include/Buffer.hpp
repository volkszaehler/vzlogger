/**
 * Circular buffer (double linked, threadsafe)
 *
 * Used to store recent readings and buffer in case of net inconnectivity
 *
 * @author Steffen Vogel <info@steffenvogel.de>
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
 * @package vzlogger
 * @license http://opensource.org/licenses/gpl-license.php GNU Public License
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

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <list>
#include <pthread.h>
#include <sys/time.h>

#include <Reading.hpp>

class Buffer {

  public:
	typedef vz::shared_ptr<Buffer> Ptr;
	typedef std::list<Reading>::iterator iterator;
	typedef std::list<Reading>::const_iterator const_iterator;

	enum aggmode { NONE, MAX, AVG, SUM };

	Buffer();
	virtual ~Buffer();

	void aggregate(int aggtime, bool aggFixedInterval);
	void push(const Reading &rd);
	void clean(bool deleted_only = true);
	void undelete();
	void shrink(/*size_t keep = 0*/);
	std::string dump();

	inline iterator begin() { return _sent.begin(); }
	inline iterator end() { return _sent.end(); }
	inline size_t size() {
		lock();
		size_t s = _sent.size();
		unlock();
		return s;
	}

	inline bool newValues() const { return _newValues; }
	inline void clear_newValues() { _newValues = false; }

	inline void lock() { pthread_mutex_lock(&_mutex); }
	inline void unlock() { pthread_mutex_unlock(&_mutex); }
	inline void wait(pthread_cond_t *condition) { pthread_cond_wait(condition, &_mutex); }

	inline void have_newValues() { _newValues = true; }

	inline void set_aggmode(Buffer::aggmode m) { _aggmode = m; }
	inline aggmode get_aggmode() const { return _aggmode; }

  private:
	Buffer(const Buffer &);            // don't allow copy constructor
	Buffer &operator=(const Buffer &); // and no assignment op.

	std::list<Reading> _sent;
	bool _newValues;

	Buffer::aggmode _aggmode;

	size_t _keep; /**< number of readings to cache for local interface */

	pthread_mutex_t _mutex;

	Reading
		*_last_avg; // keeps value and time from last reading from aggregate call for aggmode AVG
};

#endif /* _BUFFER_H_ */
