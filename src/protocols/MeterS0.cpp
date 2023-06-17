/**
 * S0 Hutschienenzähler directly connected to an rs232 port
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

#include <errno.h>
#include <fcntl.h>
#include <gpiod.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "threads.h"

#include "Options.hpp"
#include "protocols/MeterS0.hpp"
#include <VZException.hpp>

MeterS0::MeterS0(std::list<Option> options, HWIF *hwif, HWIF *hwif_dir)
	: Protocol("s0"), _hwif(hwif), _hwif_dir(hwif_dir), _counter_thread_stop(false),
	  _send_zero(false), _debounce_delay_ms(0), _nonblocking_delay_ns(1e5), _first_impulse(true) {
	OptionList optlist;

	// check which HWIF to use:
	// if "gpio" is given -> GPIO
	// else (assuming "device") -> UART
	bool use_gpio = false;
	bool use_mmap = false;
	std::string mmap;
	int gpiopin = -1;

	if (!_hwif) {
		try {
			gpiopin = optlist.lookup_int(options, "gpio");
			if (gpiopin >= 0)
				use_gpio = true;
		} catch (vz::VZException &e) {
			// ignore
		}

		if (use_gpio) {
			try {
				mmap = optlist.lookup_string(options, "mmap");
				if (mmap == "rpi2" || mmap == "rpi" || mmap == "rpi1") {
					use_mmap = true;
				} else {
					print(log_error, "unknown option for mmap (%s). Falling back to normal gpio.",
						  name().c_str(), mmap.c_str());
				}
			} catch (vz::VZException &e) {
				// ignore
			}
			if (use_mmap) {
				_hwif = new HWIF_MMAP(gpiopin, mmap);
			} else if (gpiopin > 1000) {
				print(log_info, "GPIO Pin %d greater than 1000. Using GPIOD interface for gpio.",
					  name().c_str(), gpiopin);
				_hwif = new HWIF_GPIOD(gpiopin - 1000, options);
			} else
				_hwif = new HWIF_GPIO(gpiopin, options);
		} else {
			_hwif = new HWIF_UART(options);
		}
	}

	if (!_hwif_dir) {
		int gpiodirpin = -1;
		try {
			gpiodirpin = optlist.lookup_int(options, "gpio_dir");
		} catch (vz::VZException &e) {
			// ignore
		}
		if (gpiodirpin >= 0) {
			if (gpiodirpin == gpiopin) {
				throw vz::VZException("gpio_dir must not be equal to gpio");
			}
			if (use_mmap) {
				_hwif_dir = new HWIF_MMAP(gpiodirpin, mmap);
			} else if (gpiopin > 1000) {
				print(log_info,
					  "GPIO Pin %d greater than 1000. Using GPIOD interface for gpio_dir.",
					  name().c_str(), gpiopin);
				_hwif_dir = new HWIF_GPIOD(gpiodirpin - 1000, options);
			} else
				_hwif_dir = new HWIF_GPIO(gpiodirpin, options);
		}
	}

	try {
		_resolution = optlist.lookup_int(options, "resolution");
	} catch (vz::OptionNotFoundException &e) {
		_resolution = 1000;
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse resolution", "");
		throw;
	}
	if (_resolution < 1)
		throw vz::VZException("Resolution must be greater than 0.");

	try {
		_debounce_delay_ms = optlist.lookup_int(options, "debounce_delay");
	} catch (vz::OptionNotFoundException &e) {
		_debounce_delay_ms = 30;
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse debounce_delay", "");
		throw;
	}
	if (_debounce_delay_ms < 0)
		throw vz::VZException("debounce_delay must not be negative.");

	try {
		_nonblocking_delay_ns = optlist.lookup_int(options, "nonblocking_delay");
	} catch (vz::OptionNotFoundException &e) {
		// keep default 1e5;
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse nonblocking_delay", "");
		throw;
	}
	if (_nonblocking_delay_ns < 100)
		throw vz::VZException("nonblocking_delay must not be <100ns.");

	try {
		_send_zero = optlist.lookup_bool(options, "send_zero");
	} catch (vz::OptionNotFoundException &e) {
		// keep default init value (false)
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse send_zero", "");
		throw;
	}
}

MeterS0::~MeterS0() {
	if (_hwif)
		delete _hwif;
	if (_hwif_dir)
		delete _hwif_dir;
}

void timespec_sub(const struct timespec &a, const struct timespec &b, struct timespec &res) {
	res.tv_sec = a.tv_sec - b.tv_sec;
	res.tv_nsec = a.tv_nsec - b.tv_nsec;
	if (res.tv_nsec < 0) {
		--res.tv_sec;
		res.tv_nsec += 1000000000ul;
	}
}

void timespec_add(struct timespec &a, const struct timespec &b) {
	a.tv_sec += b.tv_sec;
	a.tv_nsec += b.tv_nsec;
	// normalize nsec
	while (a.tv_nsec >= 1000000000l) {
		++a.tv_sec;
		a.tv_nsec -= 1000000000l;
	}
}

unsigned long timespec_sub_ms(const struct timespec &a, const struct timespec &b) {
	unsigned long ret;
	ret = a.tv_sec - b.tv_sec;
	if (a.tv_nsec < b.tv_nsec) {
		--ret;
		ret *= 1000ul;
		ret += (1000000000ul + a.tv_nsec - b.tv_nsec) / 1000000ul;
	} else {
		ret *= 1000ul;
		ret += (a.tv_nsec - b.tv_nsec) / 1000000ul;
	}
	return ret;
}

void timespec_add_ms(struct timespec &a, unsigned long ms) {
	a.tv_sec += ms / 1000ul;
	a.tv_nsec += (ms % 1000ul) * 1000000ul;
	// normalize nsec
	while (a.tv_nsec >= 1000000000l) {
		++a.tv_sec;
		a.tv_nsec -= 1000000000l;
	}
}

void MeterS0::check_ref_for_overflow() {
	// protect against _ms_last_impulse overflwing,
	// it would overflow roughly once a month with 32bit unsigned long

	if (_ms_last_impulse > (1ul << 30)) {
		// now we enter a race condition so there might be wrong impulse now!
		timespec_add_ms(_time_last_ref, 1ul << 30);
		_ms_last_impulse -= 1ul << 30;
	}
}

void MeterS0::counter_thread() {
	// _hwif exists and open() succeeded
	print(log_finest, "Counter thread started with %s hwif", name().c_str(),
		  _hwif->is_blocking() ? "blocking" : "non blocking");

	bool is_blocking = _hwif->is_blocking();

	{ // set thread priority to highest and SCHED_FIFO scheduling class
		// ignore any errors
		int policy;
		struct sched_param param;
		pthread_getschedparam(pthread_self(), &policy, &param);
		policy =
			SCHED_FIFO; // different approach would be PR_SET_TIMERSLACK with 1ns (default 50us)
		param.sched_priority = sched_get_priority_max(policy);
		if (0 != pthread_setschedparam(pthread_self(), policy, &param)) {
			print(log_alert, "failed to set policy to SCHED_FIFO for counter_thread",
				  name().c_str());
		}
	}

	// read current state from hwif: (this is needed for gpio if as well to reset waitForImpulse
	// after startup (see bug #229)
	int cur_state = _hwif->status();

	int last_state =
		(cur_state >= 0) ? cur_state : 0; // use current state if it is valid else assume low edge
	const int nonblocking_delay_ns = _nonblocking_delay_ns;
	while (!_counter_thread_stop) {
		if (is_blocking) {
			bool timeout = false;
			if (_hwif->waitForImpulse(timeout)) {
				// something has happened on the hardwareinterface (hwif)
				// because of the bouncing of the contact we still can not decide if it is a rising
				// edge event that's why we have to debounce first...
				if (!timeout && (_debounce_delay_ms > 0)) {
					// nanosleep _debounce_delay_ms
					struct timespec ts;
					ts.tv_sec = _debounce_delay_ms / 1000;
					ts.tv_nsec = (_debounce_delay_ms % 1000) * 1e6;
					struct timespec rem;
					while ((-1 == nanosleep(&ts, &rem)) && (errno == EINTR)) {
						ts = rem;
					}
				}
				//  ... and then going on with our work
				struct timespec temp_ts;
				clock_gettime(CLOCK_REALTIME, &temp_ts);
				_ms_last_impulse =
					timespec_sub_ms(temp_ts, _time_last_ref); // uses atomic operator=

				if (_hwif->status() !=
					0) { // check if value of gpio is set (or not supported/error (-1) for e.g. UART
						 // HWIF -> rising edge event (or error in case we accept the trigger)
					if (_hwif_dir && (_hwif_dir->status() >
									  0)) // check if second hardware interface has caused the event
						++_impulses_neg;
					else // main hardware interface caused the event
						++_impulses;
				}
			} else {
				if (!timeout) {
					print(log_warning, "Reading from hardwareinterface failed with %s.",
						  name().c_str(), strerror(errno));
				}
			}
		} else { // non-blocking case:
			int state = _hwif->status();
			if ((state >= 0) && (state != last_state)) {
				if (last_state == 0) { // low->high edge found
					//  auch hier muss wahrscheinlich erst das debouncing erfolgen, bevor es zur
					//  Auswertung kommt !!
					if (_hwif_dir && (_hwif_dir->status() > 0))
						++_impulses_neg;
					else
						++_impulses;
					if (_debounce_delay_ms > 0) {
						// nanosleep _debounce_delay_ms
						struct timespec ts;
						ts.tv_sec = _debounce_delay_ms / 1000;
						ts.tv_nsec = (_debounce_delay_ms % 1000) * 1e6;
						struct timespec rem;
						while ((-1 == nanosleep(&ts, &rem)) && (errno == EINTR)) {
							ts = rem;
						}
					}
				}
				last_state = state;
			} else { // error reading gpio status or status same as previous one
				struct timespec ts;
				ts.tv_sec = 0;
				ts.tv_nsec = nonblocking_delay_ns; // 5*(1e3) needed for up to 30kHz! 1e3 -> <1mhz,
												   // 1e4 -> <100kHz, 1e5 -> <10kHz
				nanosleep(&ts, NULL);              // we can ignore any errors here
			}
		} // non blocking case
	}     // while
	print(log_finest, "Counter thread stopped with %d imp", name().c_str(), _impulses.load());
}

int MeterS0::open() {

	if (!_hwif)
		return ERR;
	if (!_hwif->_open())
		return ERR;
	if (_hwif_dir && (!_hwif_dir->_open()))
		return ERR;

	_impulses = 0;
	_impulses_neg = 0;

	clock_gettime(CLOCK_REALTIME, &_time_last_read); // we use realtime as this is returned as well
													 // (clock_monotonic would be better but...)
	// store current time as last_time. Next read will return after 1s.
	_time_last_ref = _time_last_read;
	_ms_last_impulse = 0;
	_time_last_impulse_returned = _time_last_read;

	// create counter_thread and pass this as param
	_counter_thread_stop = false;
	_counter_thread = std::thread(&MeterS0::counter_thread, this);

	print(log_finest, "counter_thread created", name().c_str());

	return SUCCESS;
}

int MeterS0::close() {
	// signal thread to stop:
	_counter_thread_stop = true;
	if (_counter_thread.joinable())
		_counter_thread.join(); // wait for thread

	if (!_hwif)
		return ERR;
	if (_hwif_dir)
		_hwif_dir->_close(); // ignore errors

	return (_hwif->_close() ? SUCCESS : ERR);
}

ssize_t MeterS0::read(std::vector<Reading> &rds, size_t n) {

	ssize_t ret = 0;

	if (!_hwif)
		return 0;
	if (n < 4)
		return 0; // would be worth a debug msg!

	// wait till last+1s (even if we are already later)
	struct timespec req = _time_last_read;
	// (or even more seconds if !send_zero

	unsigned int t_imp;
	unsigned int t_imp_neg;
	bool is_zero = true;
	do {
		req.tv_sec += 1;
		CANCELLABLE(while (EINTR == clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &req, NULL)));
		// check from counter_thread the current impulses:
		t_imp = _impulses;
		t_imp_neg = _impulses_neg;
		if (t_imp > 0 || t_imp_neg > 0) {
			is_zero = false;
			// reduce _impulses to avoid wraps. there is no race cond here as it's ok if _impulses
			// is >0 afterwards if new impulses arrived in the meantime. That's why we don't set to
			// 0!
			_impulses -= t_imp;
			_impulses_neg -= t_imp_neg;
		}
	} while (!_send_zero &&
			 (is_zero)); // so we are blocking is send_zero is false and no impulse coming!
	// todo check thread cancellation on program termination

	// we got t_imp and/or t_imp_neq between _time_last_read and req

	clock_gettime(CLOCK_REALTIME, &req);
	double t1;
	double t2;
	if (_hwif->is_blocking()) {
		// if is_zero we need to correct the time here as no impulse occured!
		if (is_zero) {
			// we simply add the time from req-_time_last_read to _time_last_ref:
			struct timespec d1s;
			timespec_sub(req, _time_last_read, d1s);
			timespec_add(_time_last_ref, d1s);
			// this has a little racecond as well (if after existing while loop a impulse returned
			// the ms_last_impulse might have been increased already based on old time_last_ref
		}

		// we use the time from last impulse
		t1 = _time_last_impulse_returned.tv_sec + _time_last_impulse_returned.tv_nsec / 1e9;
		struct timespec temp_ts = _time_last_ref;
		timespec_add_ms(temp_ts, _ms_last_impulse);
		check_ref_for_overflow();
		t2 = temp_ts.tv_sec + temp_ts.tv_nsec / 1e9;
		_time_last_impulse_returned = temp_ts;
		_time_last_read = req;
		req = _time_last_impulse_returned;
	} else {
		// we use the time from last read call
		t1 = _time_last_read.tv_sec + _time_last_read.tv_nsec / 1e9;
		t2 = req.tv_sec + req.tv_nsec / 1e9;
		_time_last_read = req;
	}

	if (t2 == t1)
		t2 += 0.000001;

	if (_send_zero || t_imp > 0) {
		if (!_first_impulse) {
			double value = (3600000 / ((t2 - t1) * _resolution)) * t_imp;
			rds[ret].identifier(new StringIdentifier("Power"));
			rds[ret].time(req);
			rds[ret].value(value);
			++ret;
		}
		rds[ret].identifier(new StringIdentifier("Impulse"));
		rds[ret].time(req);
		rds[ret].value(t_imp);
		++ret;
	}

	if (_send_zero || t_imp_neg > 0) {
		if (!_first_impulse) {
			double value = (3600000 / ((t2 - t1) * _resolution)) * t_imp_neg;
			rds[ret].identifier(new StringIdentifier("Power_neg"));
			rds[ret].time(req);
			rds[ret].value(value);
			++ret;
		}
		rds[ret].identifier(new StringIdentifier("Impulse_neg"));
		rds[ret].time(req);
		rds[ret].value(t_imp_neg);
		++ret;
	}
	if (_first_impulse && ret > 0)
		_first_impulse = false;

	print(log_finest, "Reading S0 - returning %d readings (n=%d n_neg = %d)", name().c_str(), ret,
		  t_imp, t_imp_neg);

	return ret;
}

MeterS0::HWIF_UART::HWIF_UART(const std::list<Option> &options) : _fd(-1) {
	OptionList optlist;

	try {
		_device = optlist.lookup_string(options, "device");
	} catch (vz::VZException &e) {
		print(log_alert, "Missing device or invalid type", "");
		throw;
	}
}

MeterS0::HWIF_UART::~HWIF_UART() {
	if (_fd >= 0)
		_close();
}

bool MeterS0::HWIF_UART::_open() {
	// open port
	int fd = ::open(_device.c_str(), O_RDWR | O_NOCTTY);

	if (fd < 0) {
		print(log_alert, "open(%s): %s", "", _device.c_str(), strerror(errno));
		return false;
	}

	// save current port settings
	tcgetattr(fd, &_old_tio);

	// configure port
	struct termios tio;
	memset(&tio, 0, sizeof(struct termios));

	tio.c_cflag = B300 | CS8 | CLOCAL | CREAD;
	tio.c_iflag = IGNPAR;
	tio.c_oflag = 0;
	tio.c_lflag = 0;
	tio.c_cc[VMIN] = 0;
	tio.c_cc[VTIME] = 10; // 1s timeout see man 3 termios

	tcflush(fd, TCIFLUSH);

	// apply configuration
	tcsetattr(fd, TCSANOW, &tio);
	_fd = fd;

	return true;
}

bool MeterS0::HWIF_UART::_close() {
	if (_fd < 0)
		return false;

	tcsetattr(_fd, TCSANOW, &_old_tio); // reset serial port

	::close(_fd);
	_fd = -1;

	return true;
}

bool MeterS0::HWIF_UART::waitForImpulse(bool &timeout) {
	if (_fd < 0) {
		timeout = false;
		return false;
	}
	char buf[8];

	// clear input buffer
	tcflush(_fd, TCIOFLUSH);

	// blocking until one character/pulse is read
	ssize_t ret;
	ret = ::read(_fd, buf, 8);
	if (ret < 0) {
		timeout = false;
		return false;
	} else if (ret == 0) {
		timeout = true;
		return false;
	}
	// we don't have to set timeout here. Only in case of error.

	return true;
}

#define BLOCK_SIZE (4 * 1024)
#define BCM2708_PERI_BASE_RPI1 0x20000000
#define BCM2708_PERI_BASE_RPI2 0x3F000000

MeterS0::HWIF_MMAP::HWIF_MMAP(int gpiopin, const std::string &hw)
	: _gpiopin(gpiopin), _gpio(0), _gpio_base(0) {
	// check gpiopin for max value! (todo)
	// we do mmap in _open only
	if (hw == "rpi" || hw == "rpi1")
		_gpio_base = (void *)(BCM2708_PERI_BASE_RPI1 + 0x200000);
	else if (hw == "rpi2")
		_gpio_base = (void *)(BCM2708_PERI_BASE_RPI2 + 0x200000);
	else
		throw vz::VZException("unknown hw for HWIF_MMAP!");
}

MeterS0::HWIF_MMAP::~HWIF_MMAP() {
	if (_gpio)
		_close();
}

bool MeterS0::HWIF_MMAP::_open() {
	int mem_fd = -1;
	if ((mem_fd = ::open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		print(log_alert, "can't open /dev/mem \n", "MMAP");
		return false;
	}

	void *gpio_map = mmap((void *)NULL,           // Any adddress in our space will do
						  BLOCK_SIZE,             // Map length
						  PROT_READ | PROT_WRITE, // Enable reading & writting to mapped memory
						  MAP_SHARED,             // Shared with other processes
						  mem_fd,                 // File to map
						  (off_t)_gpio_base       // Offset to GPIO peripheral
	);

	::close(mem_fd); // No need to keep mem_fd open after mmap

	if (gpio_map == MAP_FAILED) {
		print(log_alert, "mmap error %p errno=%d\n", "MMAP", gpio_map, errno);
		return false;
	}

	// Always use volatile pointer!
	_gpio = (volatile unsigned *)gpio_map;

	return true;
}

bool MeterS0::HWIF_MMAP::_close() {
	// need unmap? todo
	_gpio = 0;
	return true;
}

int MeterS0::HWIF_MMAP::status() {
#define GET_GPIO(g) (*(_gpio + 13) & (1 << g)) // 0 if LOW, (1<<g) if HIGH

	return GET_GPIO(_gpiopin) > 0 ? 1 : 0;
}

MeterS0::HWIF_GPIO::HWIF_GPIO(int gpiopin, const std::list<Option> &options)
	: _fd(-1), _gpiopin(gpiopin), _configureGPIO(true) {
	OptionList optlist;

	if (_gpiopin < 0)
		throw vz::VZException("invalid (<0) gpio(pin) set");

	try {
		_configureGPIO = optlist.lookup_bool(options, "configureGPIO");
	} catch (vz::VZException &e) {
		print(log_info, "Missing bool configureGPIO using default true", "S0");
		_configureGPIO = true;
	}

	_device.append("/sys/class/gpio/gpio");
	_device.append(std::to_string(_gpiopin));
	_device.append("/value");
}

MeterS0::HWIF_GPIO::~HWIF_GPIO() {
	if (_fd >= 0)
		_close();
}

bool MeterS0::HWIF_GPIO::_open() {
	std::string name;
	int fd;
	unsigned int res;

	if (!::access(_device.c_str(), F_OK)) {
		// exists
	} else {
		if (_configureGPIO) {
			fd = ::open("/sys/class/gpio/export", O_WRONLY);
			if (fd < 0)
				throw vz::VZException("open export failed");
			name.clear();
			name.append(std::to_string(_gpiopin));
			name.append("\n");

			res = write(fd, name.c_str(), name.length() + 1); // is the trailing zero really needed?
			if ((name.length() + 1) != res)
				throw vz::VZException("export failed");
			::close(fd);
		} else
			return false; // doesn't exist and we shall not configure
	}

	// now it exists:
	if (_configureGPIO) {
		name.clear();
		name.append("/sys/class/gpio/gpio");
		name.append(std::to_string(_gpiopin));
		name.append("/direction");
		fd = ::open(name.c_str(), O_WRONLY);
		if (fd < 0)
			throw vz::VZException("open direction failed");
		res = ::write(fd, "in\n", 3);
		if (3 != res)
			throw vz::VZException("set direction failed");
		if (::close(fd) < 0)
			throw vz::VZException("set direction failed");

		name.clear();
		name.append("/sys/class/gpio/gpio");
		name.append(std::to_string(_gpiopin));
		name.append("/edge");
		fd = ::open(name.c_str(), O_WRONLY);
		if (fd < 0)
			throw vz::VZException("open edge failed");
		res = ::write(fd, "rising\n", 7);
		if (7 != res)
			throw vz::VZException("set edge failed");
		if (::close(fd) < 0)
			throw vz::VZException("set edge failed");

		name.clear();
		name.append("/sys/class/gpio/gpio");
		name.append(std::to_string(_gpiopin));
		name.append("/active_low");
		fd = ::open(name.c_str(), O_WRONLY);
		if (fd < 0)
			throw vz::VZException("open active_low failed");
		res = ::write(fd, "0\n", 2);
		if (2 != res)
			throw vz::VZException("set active_low failed");
		if (::close(fd) < 0)
			throw vz::VZException("set active_low failed");
	}

	fd = ::open(_device.c_str(), O_RDONLY | O_EXCL); // EXCL really needed?
	if (fd < 0) {
		print(log_alert, "open(%s): %s", "", _device.c_str(), strerror(errno));
		return false;
	}

	_fd = fd;

	return true;
}

bool MeterS0::HWIF_GPIO::_close() {
	if (_fd < 0)
		return false;

	::close(_fd);
	_fd = -1;

	return true;
}

int MeterS0::HWIF_GPIO::status() // this resets any pending events for waitForImpulse as well!
{
	unsigned char buf[2];
	if (_fd < 0)
		return -1;
	if (::pread(_fd, buf, 1, 0) < 1)
		return -2;
	if (buf[0] != '0')
		return 1;
	return 0;
}

bool MeterS0::HWIF_GPIO::waitForImpulse(bool &timeout) {
	unsigned char buf[2];
	if (_fd < 0) {
		timeout = false;
		return false;
	}

	struct pollfd poll_fd;
	poll_fd.fd = _fd;
	poll_fd.events = POLLPRI | POLLERR;
	poll_fd.revents = 0;

	int rv = poll(&poll_fd, 1, 1000); // timeout set to 1s
	print(log_debug, "MeterS0:HWIF_GPIO:first poll returned %d", "S0", rv);
	if (rv > 0) {
		if (poll_fd.revents & POLLPRI) {
			if (::pread(_fd, buf, 1, 0) < 1) {
				timeout = false;
				return false;
			}
			return true;
		} else {
			timeout = false;
			return false;
		}
	} else if (rv == 0) {
		timeout = true;
		return false;
	}
	timeout = false;
	return false;
}

/* GPIOD based interface, takes three additional config parameters:
 *
 * configureGPIO - whether or not to configure the GPIO pin active low (default: true)
 * debounce_delay (milliseconds) - it implements its own debounce logic where after an
 *                                 initial low->high or high->low transition, all additional
 *                                 transitions are ignored for a period (default: 30)
 * high_wait (milliseconds) - the minimum time the signal needs to stay high (after debounce)
 *                            before an impulse is recognized (default: -1 / disabled)
 */

MeterS0::HWIF_GPIOD::HWIF_GPIOD(int gpiopin, const std::list<Option> &options)
	: _gpiopin(gpiopin), _configureGPIO(true), _debounce_delay_ms(30), _high_count(0),
	  _high_wait_ms(-1), _chip(NULL), _line(NULL),
	  _ts_next_state_transition({.tv_sec = 0L, .tv_nsec = 0L}), _gpio_line_status(-1),
	  _state(STATE_LOW) {
	OptionList optlist;

	if (_gpiopin < 0)
		throw vz::VZException("invalid (<0) gpio(pin) set");

	try {
		_configureGPIO = optlist.lookup_bool(options, "configureGPIO");
	} catch (vz::VZException &e) {
		print(log_info, "Missing bool configureGPIO using default true", "S0");
		_configureGPIO = true;
	}

	try {
		_debounce_delay_ms = optlist.lookup_int(options, "debounce_delay");
	} catch (vz::OptionNotFoundException &e) {
		_debounce_delay_ms = 30;
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse debounce_delay", "");
		throw;
	}

	try {
		_high_wait_ms = optlist.lookup_int(options, "high_wait");
	} catch (vz::OptionNotFoundException &e) {
		_high_wait_ms = -1;
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse high_wait", "");
		throw;
	}

	print(log_info,
		  "Initialized with config gpiopin %d, configureGPIO %s, debounce_delay %d, high_wait %d",
		  "S0", _gpiopin, _configureGPIO ? "true" : "false", _debounce_delay_ms, _high_wait_ms);
}

MeterS0::HWIF_GPIOD::~HWIF_GPIOD() { _close(); }

bool MeterS0::HWIF_GPIOD::_open() {
	const char *consumername = "vzlogger-s0";
	const char *chipname = "gpiochip0"; // this is currently hardcoded since the interesting GPIO
										// lines are on chip 0 of a raspberry pi

	_chip = gpiod_chip_open_by_name(chipname);
	if (!_chip) {
		throw vz::VZException("open chip failed, errno " + std::to_string(errno));
	}

	print(log_debug, "get GPIO line %d", "S0", _gpiopin);
	_line = gpiod_chip_get_line(_chip, _gpiopin);
	if (!_line) {
		_close();
		throw vz::VZException("get line " + std::to_string(_gpiopin) + " failed, errno " +
							  std::to_string(errno));
	}

	int flags = 0;
	if (_configureGPIO) {
		print(log_info, "configuring GPIO via GPIOD (active low)", "S0");
		flags = GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;
	}

	// initialize status based on line status
	if (gpiod_line_request_input_flags(_line, consumername, flags)) {
		_close();
		throw vz::VZException("line request input failed, errno " + std::to_string(errno));
	}
	_gpio_line_status = gpiod_line_get_value(_line);
	if (_gpio_line_status == -1) {
		_close();
		throw vz::VZException("Read line status failed, errno " + std::to_string(errno));
	}
	_state = (_gpio_line_status) ? STATE_HIGH : STATE_LOW;
	gpiod_line_release(_line);

	// subscribe to GPIO events
	if (gpiod_line_request_both_edges_events_flags(_line, consumername, flags)) {
		_close();
		throw vz::VZException("line request both edge events failed, errno " +
							  std::to_string(errno));
	}

	return true;
}

bool MeterS0::HWIF_GPIOD::_close() {
	if (_line) {
		gpiod_line_release(_line);
		_line = NULL;
	}

	if (_chip) {
		gpiod_chip_close(_chip);
		_chip = NULL;
	}

	_gpio_line_status = -1;
	_state = NO_TRANSITION;

	return true;
}

/*
the GPIOD interface provides a subscription mechanism to events and guarantees that no events are
lost This means even if counter_thread sleeps, rising & falling edge events occurring will be
reported once waitForImpulse is called again. Therefore we implement our own debouncing mechanism in
a state machine.
*/
bool MeterS0::HWIF_GPIOD::waitForImpulse(bool &timeout) {
	struct gpiod_line_event event = {{0L, 0L}, -1};

	// STATE_HIGH and STATE_LOW: wait 1 sec max before returning and giving the counter thread the
	// option to exit
	struct timespec ts_max_wait_for_events = {1L, 0L};
	if (_state == STATE_DEBOUNCE || _state == STATE_HIGH_WAIT) {
		// STATE_DEBOUNCE AND STATE_HIGH_WAIT: wait max until next state transition is scheduled
		struct timespec ts_now = {0L, 0L};
		clock_gettime(CLOCK_REALTIME, &ts_now);
		timespec_sub(_ts_next_state_transition, ts_now, ts_max_wait_for_events);
	}

	// wait for GPIO events unless we're already at the next state transition
	int event_wait_result = 0;
	if (ts_max_wait_for_events.tv_sec >= 0 && ts_max_wait_for_events.tv_nsec >= 0) {
		event_wait_result = gpiod_line_event_wait(_line, &ts_max_wait_for_events);
		print(log_debug, "MeterS0:HWIF_GPIOD: wait for gpio line event returned %d", "S0",
			  event_wait_result);
	} else {
		print(log_debug, "MeterS0:HWIF_GPIOD: no wait - scheduled state transition due", "S0");
	}

	// read & handle event if one was detected: ensure that _gpio_line_status reflects the physical
	// world
	if (event_wait_result < 0) { // error
		throw vz::VZException("error waiting for event, errno " + std::to_string(errno));
	} else if (event_wait_result > 0) { // event received
		if (gpiod_line_event_read(_line, &event)) {
			throw vz::VZException("error reading event, errno " + std::to_string(errno));
		}
		print(log_info,
			  "[%ld.%ld] GPIO event %d (1=rising edge, 2=falling edge) in current state %d", "S0",
			  event.ts.tv_sec, event.ts.tv_nsec, event.event_type, _state);
		if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
			_gpio_line_status = 1;
		} else {
			_gpio_line_status = 0;
		}
	}

	// transition state machine depending on events, timeouts, gpio line state & configuration
	States newstate = NO_TRANSITION;
	switch (_state) {
	case STATE_LOW:
		if (event_wait_result > 0 && event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
			// if rising edge detected, transition to STATE_DEBOUNCE (if configured),
			// STATE_HIGH_WAIT (if configured) or directly to STATE_HIGH
			if (_debounce_delay_ms > 0) {
				newstate = STATE_DEBOUNCE;
			} else if (_high_wait_ms > 0) {
				newstate = STATE_HIGH_WAIT;
			} else {
				newstate = STATE_HIGH;
			}
		}
		break; // no state change on timeout or falling edge event

	case STATE_DEBOUNCE:
		if (event_wait_result == 0) {
			// if wait time exceeded, transition  to STATE_LOW if GPIO line is low
			// and STATE_HIGH_WAIT (if configured) or STATE_HIGH if GPIO line is high
			if (_gpio_line_status == 0) {
				newstate = STATE_LOW;
			} else if (_high_wait_ms > 0) {
				newstate = STATE_HIGH_WAIT;
			} else {
				newstate = STATE_HIGH;
			}
		}
		break; // no state change on either rising or falling edge event

	case STATE_HIGH_WAIT:
		if (event_wait_result == 0) {
			// if wait time exceeded, transition to STATE_HIGH
			newstate = STATE_HIGH;
		}
		[[fallthrough]]; // STATE_HIGH_WAIT reacts on events the same way as STATE_HIGH
	case STATE_HIGH:
		if (event_wait_result > 0 && event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
			// if falling edge detected, transition to STATE_DEBOUNCE (if configured) or directly to
			// STATE_LOW
			newstate = (_debounce_delay_ms > 0) ? STATE_DEBOUNCE : STATE_LOW;
		}
		break; // no state change on rising edge event or (in case of STATE_HIGH) timeout

	default:
		throw vz::VZException("Illegal state in state machine");
	}

	// set timestamps for automated state transitions for STATE_DEBOUNCE and STATE_HIGH_WAIT
	if (newstate == STATE_DEBOUNCE || newstate == STATE_HIGH_WAIT) {
		clock_gettime(CLOCK_REALTIME, &_ts_next_state_transition);
		timespec_add_ms(_ts_next_state_transition,
						(newstate == STATE_DEBOUNCE) ? _debounce_delay_ms : _high_wait_ms);
	} else {
		_ts_next_state_transition = {-1L, -1L};
	}

	if (newstate != NO_TRANSITION) {
		print(
			log_info,
			"MeterS0:HWIF_GPIOD: state machine transition from %d to %d with a timeout at %ld.%ld",
			"S0", _state, newstate, _ts_next_state_transition.tv_sec,
			_ts_next_state_transition.tv_nsec);
		if (newstate == STATE_HIGH) {
			_high_count++;
		} else if (newstate == STATE_LOW) {
			_high_count = 0;
		}
		_state = newstate;
	}

	// A pulse is detected at the first transition to high after a low (i.e. once _high_count is
	// exactly 1). A high->debounce->high will therefore not be counted as pulse. According to
	// https://github.com/volkszaehler/vzlogger/commit/790438d69453cc3dfd3cbcdc92fe5a9d18963a9e in
	// case of detected pulse, it should return true and set timeout to false to avoid additional
	// debouncing. In all other cases, it should return false and set timeout to true to indicate
	// that no error has happened.

	int pulse_detected = (newstate == STATE_HIGH) && (_high_count == 1);
	timeout = !pulse_detected;
	return (pulse_detected);
}
