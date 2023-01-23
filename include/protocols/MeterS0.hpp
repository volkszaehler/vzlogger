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

#ifndef _S0_H_
#define _S0_H_

#include <atomic>
#include <termios.h>
#include <thread>

#include <protocols/Protocol.hpp>

// some helper functions. might need a namespace
void timespec_sub(const struct timespec &a, const struct timespec &b, struct timespec &res);
void timespec_add(struct timespec &a, const struct timespec &b); // a+=b
void timespec_add_ms(struct timespec &a, unsigned long ms);
unsigned long timespec_sub_ms(const struct timespec &a, const struct timespec &b);

class MeterS0 : public vz::protocol::Protocol {
  public:
	class HWIF {
	  public:
		virtual ~HWIF(){};
		virtual bool _open() = 0;
		virtual bool _close() = 0;
		virtual bool waitForImpulse(bool &timeout) = 0; // blocking interface
		virtual int status() = 0; // non blocking IO status (<0 = ERR, 0 = low, 1 = high)
		virtual bool is_blocking() const = 0;
	};

	class HWIF_UART : public HWIF {
	  public:
		HWIF_UART(const std::list<Option> &options);
		virtual ~HWIF_UART();

		virtual bool _open();
		virtual bool _close();
		virtual bool waitForImpulse(bool &timeout);
		virtual int status() { return -1; }; // not supported always return error
		virtual bool is_blocking() const { return true; };

	  protected:
		std::string _device;
		int _fd;                 // file descriptor of UART
		struct termios _old_tio; // required to reset UART
	};

	class HWIF_GPIO : public HWIF {
	  public:
		HWIF_GPIO(int gpiopin, const std::list<Option> &options);
		virtual ~HWIF_GPIO();

		virtual bool _open();
		virtual bool _close();
		virtual bool waitForImpulse(bool &timeout);
		virtual int status();
		virtual bool is_blocking() const { return true; }

	  protected:
		int _fd;
		int _gpiopin;
		bool _configureGPIO; // try export,...
		std::string _device;
	};

	class HWIF_MMAP : public HWIF {
	  public:
		HWIF_MMAP(int gpiopin, const std::string &hw);
		virtual ~HWIF_MMAP();

		virtual bool _open();
		virtual bool _close();
		virtual bool waitForImpulse(bool &timeout) {
			timeout = false;
			return false;
		} // not supported
		virtual int status();
		virtual bool is_blocking() const { return false; }

	  protected:
		int _gpiopin;
		volatile unsigned *_gpio; // mmap ptr to hw registers
		void *_gpio_base;
	};

  public:
	MeterS0(std::list<Option> options, HWIF *hwif = 0, HWIF *hwif_dir = 0);
	virtual ~MeterS0();

	int open();
	int close();
	ssize_t read(std::vector<Reading> &rds, size_t n);
	virtual bool allowInterval() const {
		return false;
	} // don't allow interval setting in conf file with S0

  protected:
	void counter_thread();
	void check_ref_for_overflow();

	HWIF *_hwif;
	HWIF *_hwif_dir; // for dir gpio pin
	std::thread _counter_thread;
	std::atomic<unsigned int> _impulses;
	std::atomic<unsigned int> _impulses_neg;

	volatile bool _counter_thread_stop;

	bool _send_zero;
	int _resolution;
	int _debounce_delay_ms;
	int _nonblocking_delay_ns;

	struct timespec _time_last_read; // timestamp of last read. 1s interval based on this timestamp
	struct timespec _time_last_ref;  // reference timestamp for the millisecond delta
	std::atomic<unsigned long> _ms_last_impulse; // ms of last impulse relative to _time_last_ref
	struct timespec _time_last_impulse_returned; // timestamp of last impulse returned
	bool _first_impulse;
};

#endif /* _S0_H_ */
