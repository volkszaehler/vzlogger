/**
 * Thread headers
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

#ifndef _THREADS_H_
#define _THREADS_H_

#include <pthread.h>
#include <unistd.h>

void *logging_thread(void *arg);
void *reading_thread(void *arg);

// vzlogger uses pthread_cancel() to stop threads, which is not safe for C++ code that might invoke
// destructors, this macro is to be placed around any code that your meter spends significant
// amounts of time in, but which may not contain C++ code that might destroy objects.
// See https://blog.memzero.de/pthread-cancel-noexcept/ for details.

#define CANCELLABLE(...)                                                                           \
	do {                                                                                           \
		int oldstate;                                                                              \
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);                                  \
		__VA_ARGS__;                                                                               \
		pthread_setcancelstate(oldstate, NULL);                                                    \
	} while (0)

inline void _safe_to_cancel() { CANCELLABLE(pthread_testcancel()); }

inline void _cancellable_sleep(int seconds) { CANCELLABLE(sleep(seconds)); }

#endif /* _THREADS_H_ */
