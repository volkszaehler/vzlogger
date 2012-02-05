/**
 * Wrapper around libsml
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011, The volkszaehler.org project
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

#include "meter.h"
#include "obis.h"

using namespace std;

class MeterSML : public Meter {

public:
	MeterSML(map<string, Option> options);
	virtual ~MeterSML();

	int open();
	int close();
	int read(reading_t *rds, size_t n);

protected:
	char *host;
	char *device;
	speed_t baudrate;

	int fd;	/* file descriptor of port */
	struct termios old_tio;	/* required to reset port */

	const int BUFFER_LEN = 8192;

	/**
	 * Parses SML list entry and stores it in reading pointed by rd
	 *
	 * @param list the list entry
	 * @param rd the reading to store to
	 */
	void parse(sml_list *list, struct reading *rd);

	/**
	 * Open serial port by device
	 *
	 * @param device the device path, usually /dev/ttyS*
	 * @param old_config pointer to termios structure, will be filled with old port configuration
	 * @param baudrate the baudrate
	 * @return file descriptor, <0 on error
	 */
	int openDevice(const char *device, struct termios *old_config, speed_t baudrate);

	/**
	 * Open socket
	 *
	 * @param node the hostname or ASCII encoded IP address
	 * @param the ASCII encoded portnum or service as in /etc/services
	 * @return file descriptor, <0 on error
	 */
	int openSocket(const char *node, const char *service);
};


#endif /* _SML_H_ */
