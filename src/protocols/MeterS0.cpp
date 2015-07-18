/**
 * S0 Hutschienenzähler directly connected to an rs232 port
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

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <poll.h>

#include "protocols/MeterS0.hpp"
#include "Options.hpp"
#include <VZException.hpp>

MeterS0::MeterS0(std::list<Option> options)
		: Protocol("s0")
		, _hwif(0)
		, _counter_thread_stop(false)
		, _send_zero(false)
		, _debounce_delay_ms(0)
		, _counter(0)
{
	OptionList optlist;

	// check which HWIF to use:
	// if "gpio" is given -> GPIO
	// else (assuming "device") -> UART
	bool use_gpio = false;

	try {
		int gpiopin = optlist.lookup_int(options, "gpio");
		if (gpiopin >=0 ) use_gpio = true;
	} catch (vz::VZException &e) {
			// ignore
	}

	if (use_gpio) {
		_hwif = new HWIF_GPIO(options);
	} else {
		_hwif = new HWIF_UART(options);
	}

	try {
		_resolution = optlist.lookup_int(options, "resolution");
	} catch (vz::OptionNotFoundException &e) {
		_resolution = 1000;
	} catch (vz::VZException &e) {
		print(log_error, "Failed to parse resolution", "");
		throw;
	}
	if (_resolution < 1) throw vz::VZException("Resolution must be greater than 0.");

	try {
		_debounce_delay_ms = optlist.lookup_int(options, "debounce_delay");
	} catch (vz::OptionNotFoundException &e) {
		_debounce_delay_ms = 30;
	} catch (vz::VZException &e) {
		print(log_error, "Failed to parse debounce_delay", "");
		throw;
	}
	if (_debounce_delay_ms < 0) throw vz::VZException("debounce_delay must not be negative.");

	try {
		_send_zero = optlist.lookup_bool(options, "send_zero");
	} catch (vz::OptionNotFoundException &e) {
		// keep default init value (false)
	} catch (vz::VZException &e) {
		print(log_error, "Failed to parse send_zero", "");
		throw;
	}

}

MeterS0::~MeterS0()
{
	if (_hwif) delete _hwif;
}

void timespec_sub(const struct timespec &a, const struct timespec &b, struct timespec &res)
{
	res.tv_sec = a.tv_sec - b.tv_sec;
	res.tv_nsec = a.tv_nsec - b.tv_nsec;
	if (res.tv_nsec < 0) {
		--res.tv_sec;
		res.tv_nsec += 1e9;
	}
}

void MeterS0::counter_thread()
{
	// _hwif exists and open() succeeded
	print(log_finest, "Counter thread started with %s hwif", name().c_str(), _hwif->is_blocking() ? "blocking" : "non blocking");
	bool is_blocking = _hwif->is_blocking();
	while(!_counter_thread_stop) {
		if (is_blocking) {
			if (_hwif->waitForImpulse())
				++_impulses; // todo add logic for _impulses_neg
			// we handle errors from waitForImpulse by simply debouncing and trying again (with debounce_delay_ms==0 this might be an endless loop
			if (_debounce_delay_ms > 0){
				// nanosleep _debounde_delay_ms
				struct timespec ts;
				ts.tv_sec = _debounce_delay_ms/1000;
				ts.tv_nsec = (_debounce_delay_ms%1000)*1e6;
				struct timespec rem;
				while ( (-1 == nanosleep(&ts, &rem)) && (errno == EINTR) ) {
					ts = rem;
				}
			}
		} else {
			// non blocking
			usleep(1000); // fixme update/increase _impulses and _impulses_neg here (ignore wrap around as read will reduce them

		}
	}
	print(log_finest, "Counter thread stopped", name().c_str());
}

int MeterS0::open() {

	if (!_hwif) return ERR;
	if (!_hwif->_open()) return ERR;

	_impulses = 0;
	_impulses_neg = 0;

	// create counter_thread and pass this as param
	_counter_thread_stop = false;
	_counter_thread = std::thread(&MeterS0::counter_thread, this);

	// todo increase counter_threads scheduling class and prio!

	clock_gettime(CLOCK_REALTIME, &_time_last); // we use realtime as this is returned as well (clock_monotonic would be better but...)
	// store current time as last_time. Next read will return after 1s.

	print(log_finest, "counter_thread created", name().c_str());

	return SUCCESS;
}

int MeterS0::close() {
	// signal thread to stop:
	_counter_thread_stop = true;
	if (_counter_thread.joinable())
		_counter_thread.join(); // wait for thread

	if (!_hwif) return ERR;

	return (_hwif->_close() ? SUCCESS : ERR);
}

ssize_t MeterS0::read(std::vector<Reading> &rds, size_t n) {

	ssize_t ret = 0;

	if (!_hwif) return 0;
	if (n<2) return 0; // would be worth a debug msg!

	// wait till last+1s (even if we are already later)
	struct timespec req = _time_last;
	// (or even more seconds if !send_zero

	unsigned int t_imp;
	unsigned int t_imp_neg;
	bool is_zero = true;
	do{
		req.tv_sec += 1;
		while (EINTR == clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &req, NULL));
		// check from counter_thread the current impulses:
		t_imp = _impulses;
		t_imp_neg = _impulses_neg;
		if (t_imp > 0 || t_imp_neg > 0 ) {
			is_zero = false;
			// reduce _impulses to avoid wraps. there is no race cond here as it's ok if _impulses is >0 afterwards if new impulses arrived in the meantime. That's why we don't set to 0!
			_impulses -= t_imp;
			_impulses_neg -= t_imp_neg;
		}
	} while (!_send_zero && (is_zero)); // so we are blocking is send_zero is false and no impulse coming!
	// todo check thread cancellation on program termination

	// we got t_imp and/or t_imp_neq between _time_last and req

	double t1 = _time_last.tv_sec + _time_last.tv_nsec / 1e9;
	double t2 = req.tv_sec + req.tv_nsec / 1e9;

	_time_last = req;

	if (_send_zero || t_imp > 0 ) {
		double value = 3600000 / ((t2-t1) * (_resolution * t_imp));
		rds[ret].identifier(new StringIdentifier("Power"));
		rds[ret].time(req);
		rds[ret].value(value);
		++ret;
		rds[ret].identifier(new StringIdentifier("Impulse"));
		rds[ret].time(req);
		rds[ret].value(t_imp);
		++ret;
	}

	if (_send_zero || t_imp_neg > 0) {
		double value = 3600000 / ((t2-t1) * (_resolution * t_imp_neg));
		rds[ret].identifier(new StringIdentifier("Power_neg"));
		rds[ret].time(req);
		rds[ret].value(value);
		++ret;
		rds[ret].identifier(new StringIdentifier("Impulse_neg"));
		rds[ret].time(req);
		rds[ret].value(t_imp_neg);
		++ret;
	}

	print(log_finest, "Reading S0 - n=%d n_neg = %d", name().c_str(), t_imp, t_imp_neg);

	return ret;
}

MeterS0::HWIF_UART::HWIF_UART(const std::list<Option> &options) :
	_fd(-1)
{
	OptionList optlist;

	try {
		_device = optlist.lookup_string(options, "device");
	} catch (vz::VZException &e) {
		print(log_error, "Missing device or invalid type", "");
		throw;
	}
}

MeterS0::HWIF_UART::~HWIF_UART()
{
	if (_fd>=0) _close();
}

bool MeterS0::HWIF_UART::_open()
{
	// open port
	int fd = ::open(_device.c_str(), O_RDWR | O_NOCTTY);

	if (fd < 0) {
		print(log_error, "open(%s): %s", "", _device.c_str(), strerror(errno));
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
	tio.c_cc[VMIN]=1;
	tio.c_cc[VTIME]=0;

	tcflush(fd, TCIFLUSH);

	// apply configuration
	tcsetattr(fd, TCSANOW, &tio);
	_fd = fd;

	return true;
}

bool MeterS0::HWIF_UART::_close()
{
	if (_fd<0) return false;

	tcsetattr(_fd, TCSANOW, &_old_tio); // reset serial port

	::close(_fd);
	_fd = -1;

	return true;
}

bool MeterS0::HWIF_UART::waitForImpulse()
{
	if (_fd<0) return false;
	char buf[8];

	// clear input buffer
	tcflush(_fd, TCIOFLUSH);

	// blocking until one character/pulse is read
	if (::read(_fd, buf, 8) < 1) return false;

	return true;
}

MeterS0::HWIF_GPIO::HWIF_GPIO(const std::list<Option> &options) :
	_fd(-1), _gpiopin(-1), _configureGPIO(true)
{
	OptionList optlist;

	try {
		_gpiopin = optlist.lookup_int(options, "gpio");
	} catch (vz::VZException &e) {
		print(log_error, "Missing gpio or invalid type (expect int)", "S0");
		throw;
	}
	if (_gpiopin <0 ) throw vz::VZException("invalid (<0) gpio(pin) set");

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

MeterS0::HWIF_GPIO::~HWIF_GPIO()
{
	if (_fd>=0) _close();
}

bool MeterS0::HWIF_GPIO::_open()
{
	std::string name;
	int fd;
	unsigned int res;

	if (!::access(_device.c_str(),F_OK)){
		// exists
	} else {
		if (_configureGPIO) {
			fd = ::open("/sys/class/gpio/export",O_WRONLY);
			if (fd<0) throw vz::VZException("open export failed");
			name.clear();
			name.append(std::to_string(_gpiopin));
			name.append("\n");

			res=write(fd,name.c_str(), name.length()+1); // is the trailing zero really needed?
			if ((name.length()+1)!=res) throw vz::VZException("export failed");
			::close(fd);
		} else return false; // doesn't exist and we shall not configure
	}

	// now it exists:
	if (_configureGPIO) {
		name.clear();
		name.append("/sys/class/gpio/gpio");
		name.append(std::to_string(_gpiopin));
		name.append("/direction");
		fd = ::open(name.c_str(), O_WRONLY);
		if (fd<0) throw vz::VZException("open direction failed");
		res=::write(fd,"in\n",3);
		if (3!=res) throw vz::VZException("set direction failed");
		if (::close(fd)<0) throw vz::VZException("set direction failed");

		name.clear();
		name.append("/sys/class/gpio/gpio");
		name.append(std::to_string(_gpiopin));
		name.append("/edge");
		fd = ::open(name.c_str(), O_WRONLY);
		if (fd<0) throw vz::VZException("open edge failed");
		res=::write(fd,"rising\n",7);
		if (7!=res) throw vz::VZException("set edge failed");
		if (::close(fd)<0) throw vz::VZException("set edge failed");

		name.clear();
		name.append("/sys/class/gpio/gpio");
		name.append(std::to_string(_gpiopin));
		name.append("/active_low");
		fd = ::open(name.c_str(), O_WRONLY);
		if (fd<0) throw vz::VZException("open active_low failed");
		res=::write(fd,"0\n",2);
		if (2!=res) throw vz::VZException("set active_low failed");
		if (::close(fd)<0) throw vz::VZException("set active_low failed");
	}

	fd = ::open( _device.c_str(), O_RDONLY | O_EXCL); // EXCL really needed?
	if (fd < 0) {
		print(log_error, "open(%s): %s", "", _device.c_str(), strerror(errno));
		return false;
	}

	_fd = fd;

	return true;
}

bool MeterS0::HWIF_GPIO::_close()
{
	if (_fd<0) return false;

	::close(_fd);
	_fd = -1;

	return true;
}

bool MeterS0::HWIF_GPIO::waitForImpulse()
{
	unsigned char buf[2];
	if (_fd<0) return false;

	struct pollfd poll_fd;
	poll_fd.fd = _fd;
	poll_fd.events = POLLPRI|POLLERR;
	poll_fd.revents = 0;

	int rv = poll(&poll_fd, 1, -1);    // block endlessly
	print(log_debug, "MeterS0:HWIF_GPIO:first poll returned %d", "S0", rv);
	if (rv > 0) {
		if (poll_fd.revents & POLLPRI) {
			if (::pread(_fd, buf, 1, 0) < 1) return false;
		} else return false;
	} else return false;

	return true;
}
