/**
 * Wrapper around libsml
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
 * @copyright Copyright (c) 2011, DAI-Labor, TU-Berlin
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
 * @author Juri Glass
 * @author Mathias Runge
 * @author Nadim El Sayed
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

#ifndef _SML_H_
#define _SML_H_

#include <sml/sml_file.h>
#include <sml/sml_value.h>

#include <termios.h>

#include "Obis.hpp"
#include <protocols/Protocol.hpp>

class MeterSML : public vz::protocol::Protocol {

  public:
	MeterSML(std::list<Option> options);
	MeterSML(const MeterSML &mtr);
	virtual ~MeterSML();

	// MeterSML& operator=(const MeterSML&proto) {  std::cout<<"====>MeterSML - equal!" <<
	// std::endl; return (*this); }

	int open();
	int close();
	ssize_t read(std::vector<Reading> &rds, size_t n);
	virtual bool allowInterval() const {
		return false;
	} // don't allow conf setting interval with sml

	const char *host() const { return _host.c_str(); }
	const char *device() const { return _device.c_str(); }

  protected:
	std::string _host;
	std::string _device;
	speed_t _baudrate;
	parity_type_t _parity;
	std::string _pull;
	bool _use_local_time;

	int _fd;                 /* file descriptor of port */
	struct termios _old_tio; /* required to reset port */

	const int BUFFER_LEN;

	/**
	 * @brief reopen the underlying device. We do this to workaround issue #362
	 * @return true if reopen was successful. False otherwise.
	 * */
	bool reopen();

	/**
	 * Parses SML list entry and stores it in reading pointed by rd
	 *
	 * @param list the list entry
	 * @param rd the reading to store to
	 * @return true if it is a valid entry
	 */
	bool _parse(sml_list *list, Reading *rd);

	/**
	 * Open serial port by device
	 *
	 * @param device the device path, usually /dev/ttyS*
	 * @param old_config pointer to termios structure, will be filled with old port configuration
	 * @param baudrate the baudrate
	 * @return file descriptor, <0 on error
	 */
	int _openDevice(struct termios *old_config, speed_t baudrate);

	/**
	 * Open socket
	 *
	 * @param node the hostname or ASCII encoded IP address
	 * @param the ASCII encoded portnum or service as in /etc/services
	 * @return file descriptor, <0 on error
	 */
	int _openSocket(const char *node, const char *service);
};

#endif /* _SML_H_ */
