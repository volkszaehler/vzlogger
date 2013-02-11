/**
 * Circular buffer (double linked, threadsafe)
 *
 * Used to store recent readings and buffer in case of net inconnectivity
 *
 * @author Steffen Vogel <info@steffenvogel.de>
 * @copyright Copyright (c) 2011, The volkszaehler.org project
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

#include <pthread.h>
#include <sys/time.h>
#include <list>

#include <Reading.hpp>

class Buffer {

	public:
	typedef vz::shared_ptr<Buffer> Ptr;
	typedef std::list<Reading>::iterator iterator;
	typedef std::list<Reading>::const_iterator const_iterator;

	Buffer();
	virtual ~Buffer();

	void push(const Reading &rd);
	void clean();
	void undelete();
	void shrink(/*size_t keep = 0*/);
	char *dump(char *dump, size_t len);

	inline iterator begin() { return _sent.begin(); }
	inline iterator end()   { return _sent.end(); }
	inline size_t size() { return _sent.size(); }

	inline const bool newValues() const { return _newValues; }
	inline void clear_newValues() { _newValues = false; }

	inline const size_t keep() { return _keep; }
	inline void keep(const size_t keep) { _keep = keep; }

	inline void lock()   { pthread_mutex_lock(&_mutex); }
	inline void unlock() { pthread_mutex_unlock(&_mutex); }
	inline void wait(pthread_cond_t *condition) { pthread_cond_wait(condition, &_mutex); }

	private:
	inline void have_newValues() { _newValues =  true; }

	private:
	std::list<Reading> _sent;
	bool _newValues;

	size_t _keep;	/**< number of readings to cache for local interface */

	pthread_mutex_t _mutex;
};

#endif /* _BUFFER_H_ */


/*
 * Local variables:
 *  tab-width: 2
 *  c-indent-level: 2
 *  c-basic-offset: 2
 *  project-name: vzlogger
 * End:
 */
