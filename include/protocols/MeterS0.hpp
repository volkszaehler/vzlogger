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

#ifndef _S0_H_
#define _S0_H_

#include <thread>
#include <atomic>
#include <termios.h>

#include <protocols/Protocol.hpp>

class MeterS0 : public vz::protocol::Protocol {

	class HWIF {
	public:
		virtual ~HWIF() {};
		virtual bool _open() = 0;
		virtual bool _close() = 0;
		virtual bool waitForImpulse() = 0;
		virtual bool is_blocking() const = 0;
	};

	class HWIF_UART : public HWIF {
	public:
		HWIF_UART(const std::list<Option> &options);
		virtual ~HWIF_UART();

		virtual bool _open();
		virtual bool _close();
		virtual bool waitForImpulse();
		virtual bool is_blocking() const { return true; };
	protected:
		std::string _device;
		int _fd;					// file descriptor of UART
		struct termios _old_tio;	// required to reset UART
	};

	class HWIF_GPIO : public HWIF {
	public:
		HWIF_GPIO(const std::list<Option> &options);
		virtual ~HWIF_GPIO();

		virtual bool _open();
		virtual bool _close();
		virtual bool waitForImpulse();
		virtual bool is_blocking() const { return true; }
	protected:
		int _fd;
		int _gpiopin;
		bool _configureGPIO; // try export,...
		std::string _device;
	};

public:
	MeterS0(std::list<Option> options);
	virtual ~MeterS0();

	int open();
	int close();
	ssize_t read(std::vector<Reading> &rds, size_t n);

  protected:
	void counter_thread();

    HWIF * _hwif;
	std::thread _counter_thread;
	std::atomic<unsigned int> _impulses;
	std::atomic<unsigned int> _impulses_neg;

	volatile bool _counter_thread_stop;

	bool _send_zero;
	int _resolution;
	int _debounce_delay_ms;
	int _counter;

	struct timespec _time_last;	// timestamp of last impulse
};

#endif /* _S0_H_ */
