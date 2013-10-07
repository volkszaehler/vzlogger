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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

/* serial port */
#include <fcntl.h>
#include <sys/ioctl.h>

/* socket */
#include <netdb.h>
#include <sys/socket.h>

/* sml stuff */
#include <sml/sml_file.h>
#include <sml/sml_transport.h>

#include "protocols/MeterSML.hpp"
#include "Obis.hpp"
#include "Options.hpp"
#include <VZException.hpp>

#define SML_BUFFER_LEN 8096

MeterSML::MeterSML(std::list<Option> options) 
		: Protocol("sml")
		, _host("")
		, _device("")
		, BUFFER_LEN(SML_BUFFER_LEN)
{
	OptionList optlist;

	/* connection */
	try {
		_host = optlist.lookup_string(options, "host");
	} catch ( vz::OptionNotFoundException &e ) {
		try {
			_device = optlist.lookup_string(options, "device");
		} catch ( vz::VZException &e ){
			print(log_error, "Missing device or host", name().c_str());
			throw ;
		}
	} catch( vz::VZException &e ) {
		print(log_error, "Missing device or host", name().c_str());
		throw;
	}
	try {
		std::string hex;
		hex = optlist.lookup_string(options, "pullseq");
		int n=hex.size();
		int i;
		for(i=0;i<n;i=i+2) {
			char hs[3];
			strncpy(hs,hex.c_str()+i,2);
			char hx[2];
			hx[0]=strtol(hs,NULL,16);
			_pull.append(hx,1);
		}
		print(log_debug,"pullseq len:%d found",name().c_str(),_pull.size());
	} catch( vz::OptionNotFoundException &e ) {
		/* using default value if not specified */
		_pull = "";
	}


	/* baudrate */
	int baudrate = 9600; /* default to avoid compiler warning */
	try {
		baudrate = optlist.lookup_int(options, "baudrate");
		/* find constant for termios structure */
		switch (baudrate) {
				case 50: _baudrate = B50; break;
				case 75: _baudrate = B75; break;
				case 110: _baudrate = B110; break;
				case 134: _baudrate = B134; break;
				case 150: _baudrate = B150; break;
				case 200: _baudrate = B200; break;
				case 300: _baudrate = B300; break;
				case 600: _baudrate = B600; break;
				case 1200: _baudrate = B1200; break;
				case 1800: _baudrate = B1800; break;
				case 2400: _baudrate = B2400; break;
				case 4800: _baudrate = B4800; break;
				case 9600: _baudrate = B9600; break;
				case 19200: _baudrate = B19200; break;
				case 38400: _baudrate = B38400; break;
				case 57600: _baudrate = B57600; break;
				case 115200: _baudrate = B115200; break;
				case 230400: _baudrate = B230400; break;
				default:
					print(log_error, "Invalid baudrate: %i", name().c_str(), baudrate);
					throw vz::VZException("Invalid baudrate");
		}
	} catch( vz::OptionNotFoundException &e ) {
		/* using default value if not specified */
		_baudrate = B9600;
	} catch( vz::VZException &e ) {
		print(log_error, "Failed to parse the baudrate", name().c_str());
		throw;
	}

	_parity=parity_8n1;
	try {
		const char *parity = optlist.lookup_string(options, "parity");
		/* find constant for termios structure */
		if(strcasecmp(parity,"8n1")==0) {
			_parity=parity_8n1;
		} else if(strcasecmp(parity,"7n1")==0) {
			_parity=parity_7n1;
		} else if(strcasecmp(parity,"7e1")==0) {
			_parity=parity_7e1;
		} else if(strcasecmp(parity,"7o1")==0) {
			_parity=parity_7o1;
		} else {
			throw vz::VZException("Invalid parity");
		}
	} catch( vz::OptionNotFoundException &e ) {
		/* using default value if not specified */
		_parity = parity_8n1;
	} catch( vz::VZException &e ) {
		print(log_error, "Failed to parse the parity", name().c_str());
		throw;
	}

}

MeterSML::MeterSML(const MeterSML &proto)
		: Protocol(proto)
		, BUFFER_LEN(SML_BUFFER_LEN)
{
}

MeterSML::~MeterSML() {
}

int MeterSML::open() {
	
	if (_device != "") {
		_fd = _openDevice(&_old_tio, _baudrate);
	}
	else if (_host != "") {
		char *addr = strdup(host());
		const char *node = strsep(&addr, ":");
		const char *service = strsep(&addr, ":");
		if(node == NULL && service == NULL) return -1;
		_fd = _openSocket(node, service);
		free(addr);
	}
	return _fd;
}

int MeterSML::close() {

	if (_device != "") {
		/* reset serial port */
		tcsetattr(_fd, TCSANOW, &_old_tio);
	}

	return ::close(_fd);
}

ssize_t MeterSML::read(std::vector<Reading> &rds, size_t n) {

	unsigned char buffer[SML_BUFFER_LEN];
	size_t bytes, m = 0;

	sml_file *file;
	sml_get_list_response *body;
	sml_list *entry;

	if(_pull.size()) {
		int wlen=write(_fd,_pull.c_str(),_pull.size());
		print(log_debug,"sending pullsequenz send (len:%d is:%d).",name().c_str(),_pull.size(),wlen);
	}

	/* wait until a we receive a new datagram from the meter (blocking read) */
	bytes = sml_transport_read(_fd, buffer, SML_BUFFER_LEN);

	if(bytes < 16 ) {
		print(log_error, "short message from sml_transport_read len=%d", name().c_str(), bytes);
		return(0);
	}

	/* parse SML file & stripping escape sequences */
	file = sml_file_parse(buffer + 8, bytes - 16);

	/* obtain SML messagebody of type getResponseList */
	for (short i = 0; i < file->messages_len; i++) {
		sml_message *message = file->messages[i];

		if (*message->message_body->tag == SML_MESSAGE_GET_LIST_RESPONSE) {
			body = (sml_get_list_response *) message->message_body->data;
			entry = body->val_list;

			/* iterating through linked list */
			for (m = 0; m < n && entry != NULL; m++) {
				_parse(entry, &rds[m]);
				entry = entry->next;
			}
		}
	}

	/* free the malloc'd memory */
	sml_file_free(file);

	return m+1;
}

void MeterSML::_parse(sml_list *entry, Reading *rd) {
	//int unit = (entry->unit) ? *entry->unit : 0;
	int scaler = (entry->scaler) ? *entry->scaler : 1;
	
	rd->value(sml_value_to_double(entry->value) * pow(10, scaler));

	Obis obis((unsigned char)entry->obj_name->str[0],
						(unsigned char)entry->obj_name->str[1],
						(unsigned char)entry->obj_name->str[2],
						(unsigned char)entry->obj_name->str[3],
						(unsigned char)entry->obj_name->str[4],
						(unsigned char)entry->obj_name->str[5]);
	ReadingIdentifier *rid(new ObisIdentifier(obis));
	rd->identifier(rid);
	
	// TODO handle SML_TIME_SEC_INDEX or time by SML File/Message
	struct timeval tv;
	if (entry->val_time) { /* use time from meter */
		tv.tv_sec = *entry->val_time->data.timestamp;
		tv.tv_usec = 0;
	}
	else {
		gettimeofday(&tv, NULL); /* use local time */
	}
	rd->time(tv);
}

int MeterSML::_openSocket(const char *node, const char *service) {
	struct sockaddr_in sin;
	struct addrinfo *ais;
	int fd, res;

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		print(log_error, "socket(): %s", name().c_str(), strerror(errno));
		return ERR;
	}

	int rc = getaddrinfo(node, service, NULL, &ais);
	if (rc != 0) {
		print(log_error, "getaddrinfo(%s, %s): %s", name().c_str(), node, service, gai_strerror(rc));
		return ERR;
	}

	memcpy(&sin, ais->ai_addr, ais->ai_addrlen);
	freeaddrinfo(ais);

	res = connect(fd, (struct sockaddr *) &sin, sizeof(sin));
	if (res < 0) {
		print(log_error, "connect(%s, %s): %s", name().c_str(), node, service, strerror(errno));
		return ERR;
	}

	return fd;
}

int MeterSML::_openDevice(struct termios *old_tio, speed_t baudrate) {
	int bits;
	struct termios tio;
	memset(&tio, 0, sizeof(struct termios));

	int fd = ::open(device(), O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0) {
		print(log_error, "open(%s): %s", name().c_str(), device(), strerror(errno));
		return ERR;
	}

	/* enable RTS as supply for infrared adapters */
	ioctl(fd, TIOCMGET, &bits);
	bits |= TIOCM_RTS;
	ioctl(fd, TIOCMSET, &bits);

	/* get old configuration */
	tcgetattr(fd, &tio) ;

	/* backup old configuration to restore it when closing the meter connection */
	memcpy(old_tio, &tio, sizeof(struct termios));

	tio.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tio.c_oflag &= ~OPOST;
	tio.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

	switch(_parity) {
	case parity_8n1:
		tio.c_cflag &= ~ PARENB;
		tio.c_cflag &= ~ CSTOPB;
		tio.c_cflag &= ~ CSIZE;
		tio.c_cflag |= CS8;
		break;
	case parity_7n1:
		tio.c_cflag &= ~ PARENB;
		tio.c_cflag &= ~ CSTOPB;
		tio.c_cflag &= ~ CSIZE;
		tio.c_cflag |= CS7;
		break;
	case parity_7e1:
		tio.c_cflag |= ~ PARENB;
		tio.c_cflag &= ~ PARODD;
		tio.c_cflag &= ~ CSTOPB;
		tio.c_cflag &= ~ CSIZE;
		tio.c_cflag |= CS7;
		break;
	case parity_7o1:
		tio.c_cflag |= ~ PARENB;
		tio.c_cflag |= ~ PARODD;
		tio.c_cflag &= ~ CSTOPB;
		tio.c_cflag &= ~ CSIZE;
		tio.c_cflag |= CS7;
		break;
	}

	/* set baudrate */
	cfsetispeed(&tio, baudrate);
	cfsetospeed(&tio, baudrate);

	/* apply new configuration */
	tcsetattr(fd, TCSANOW, &tio);

	return fd;
}
