/**
 * Plaintext protocol according to DIN EN 62056-21
 *
 * This protocol uses OBIS codes to identify the readout data
 *
 * This is our example protocol. Use this skeleton to add your own
 * protocols and meters.
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
 * @author Mathias Dalheimer <md@gonium.net>
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
/*
 * Add Acknowlegde RW
 * Corrected parity
 * Correct Obidis( filter 1 und 2)
 * Filter STX
 * Baudrate change
 */

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>

/* socket */
#include <netdb.h>
#include <sys/socket.h>

#include "protocols/MeterD0.hpp"
#include <VZException.hpp>

#include "Obis.hpp"

MeterD0::MeterD0(std::list<Option> options) 
		: Protocol("d0")
		, _host("")
		, _device("")
        , _wait_sync_end (false)
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
	try {
		std::string hex;
		hex = optlist.lookup_string(options, "ackseq");
		int n=hex.size();
		int i;
		for(i=0;i<n;i=i+2) {
			char hs[3];
			strncpy(hs,hex.c_str()+i,2);
			char hx[2];
			hx[0]=strtol(hs,NULL,16);
			_ack.append(hx,1);
		}
		print(log_debug,"ackseq len:%d found %s, %x",name().c_str(),_ack.size(),_ack.c_str(),_ack.c_str()[0]);
	} catch( vz::OptionNotFoundException &e ) {
		/* using default value if not specified */
		_ack = "";
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
					print(log_error, "RW:Invalid baudrate: %i", name().c_str(), baudrate);
					throw vz::VZException("Invalid baudrate");
		}
	} catch( vz::OptionNotFoundException &e ) {
		/* using default value if not specified */
		_baudrate = B9600;
	} catch( vz::VZException &e ) {
		print(log_error, "Failed to parse the baudrate", name().c_str());
		throw;
	}
	try {
		baudrate = optlist.lookup_int(options, "baudrate_read");
		/* find constant for termios structure */
		switch (baudrate) {
				case 50:   _baudrate_read = B50; break;
				case 75:   _baudrate_read = B75; break;
				case 110:  _baudrate_read = B110; break;
				case 134:  _baudrate_read = B134; break;
				case 150:  _baudrate_read = B150; break;
				case 200:  _baudrate_read = B200; break;
				case 300:  _baudrate_read = B300; break;
				case 600:  _baudrate_read = B600; break;
				case 1200: _baudrate_read = B1200; break;
				case 1800: _baudrate_read = B1800; break;
				case 2400: _baudrate_read = B2400; break;
				case 4800: _baudrate_read = B4800; break;
				case 9600: _baudrate_read = B9600; break;
				case 19200: _baudrate_read = B19200; break;
				case 38400: _baudrate_read = B38400; break;
				case 57600: _baudrate_read = B57600; break;
				case 115200: _baudrate_read = B115200; break;
				case 230400: _baudrate_read = B230400; break;
				default:
					print(log_error, "RW:Invalid baudrate_read: %i", name().c_str(), baudrate);
					throw vz::VZException("Invalid baudrate");
		}
	} catch( vz::OptionNotFoundException &e ) {
		/* using default value if not specified */
		_baudrate_read = _baudrate;
	} catch( vz::VZException &e ) {
		print(log_error, "Failed to parse the baudrate_read", name().c_str());
		throw;
	}


	_parity=parity_7e1;
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
		_parity = parity_7e1;
	} catch( vz::VZException &e ) {
		print(log_error, "Failed to parse the parity", name().c_str());
		throw;
	}

	try {
		const char *waitsync = optlist.lookup_string(options, "wait_sync");
		if(strcasecmp(waitsync,"end")==0) {
			_wait_sync_end = true;
		} else {
			throw vz::VZException("Invalid wait_sync");
		}
	} catch( vz::OptionNotFoundException &e ) {
        /* no fault. default is off */
	} catch( vz::VZException &e ) {
		print(log_error, "Failed to parse wait_sync", name().c_str());
		throw;
	}
    

}

MeterD0::~MeterD0() {
}

int MeterD0::open() {
	if (_device != "") {
		_fd = _openDevice(&_oldtio, _baudrate);
	}
	else if (_host != "") {
		char *addr = strdup(host());
		const char *node = strsep(&addr, ":");
		const char *service = strsep(&addr, ":");

		_fd = _openSocket(node, service);
	}

	return (_fd < 0) ? ERR : SUCCESS;
}

int MeterD0::close() {
	return ::close(_fd);
}

ssize_t MeterD0::read(std::vector<Reading>&rds, size_t max_readings) {

	enum { START, VENDOR, BAUDRATE, IDENTIFICATION, ACK, START_LINE, OBIS_CODE, VALUE, UNIT, END_LINE, END } context;

	bool error_flag = false;
	char vendor[3+1];		/* 3 upper case vendor + '\0' termination */
	char identification[16+1];	/* 16 meter specific + '\0' termination */
	char obis_code[16+1];		/* A-B:C.D.E*F
														 fields A, B, E, F are optional
														 fields C & D are mandatory
														 A: energy type; 1: energy
														 B: channel number; 0: no channel specified
														 C: data items; 0-89 in COSEM context: IEC 62056-62, Clause D.1; 96: General service entries
														 1:  Totel Active power+
														 21: L1 Active power+
														 31: L1 Current
														 32: L1 Voltage
														 41: L2 Active power+
														 51: L2 Current
														 52: L2 Voltage
														 61: L3 Active power+
														 71: L3 Current
														 72: L3 Voltage
														 96.1.255: Metering point ID 256 (electricity related) 
														 96.5.5: Meter started status flag
														 D: types
														 E: further processing or classification of quantities
														 F: storage of data
														 see DIN-EN-62056-61 */
	char value[32+1];		/* value, i.e. the actual reading */
	char unit[16+1];		/* the unit of the value, e.g. kWh, V, ... */

	char baudrate;			/* 1 byte for */
	char byte,lastbyte;			/* we parse our input byte wise */
	int byte_iterator; 
	char endseq[2+1]; /* Endsequence ! not ?! */
	size_t number_of_tuples;
	struct termios tio;
	int baudrate_connect,baudrate_read;// Baudrates for switching
	
	
	baudrate_connect=_baudrate;
	baudrate_read=_baudrate_read;
	tcgetattr(_fd, &tio) ;

	if(_pull.size()) {
		tcflush(_fd, TCIOFLUSH);
		cfsetispeed(&tio, baudrate_connect);
		cfsetospeed(&tio, baudrate_connect);
		/* apply new configuration */
		tcsetattr(_fd, TCSANOW, &tio);

		int wlen=write(_fd,_pull.c_str(),_pull.size());
		print(log_debug,"sending pullsequenz send (len:%d is:%d).",name().c_str(),_pull.size(),wlen);
	}


	byte_iterator =  number_of_tuples = baudrate = 0;
	byte=lastbyte=0;	
	context = START;				/* start with context START */

    if (_wait_sync_end){
        /* wait once for the sync pattern ("!") at the end of a regular D0 message.
         This is intended for D0 meters that start sending data automatically
         (e.g. Hager EHZ361).
         */
        int skipped=0;
        while(_wait_sync_end && ::read(_fd, &byte, 1)){
            if (byte == '!'){
                _wait_sync_end=false;
                print(log_debug, "found wait_sync_end. skipped %d bytes.", skipped);
            }else{
                skipped++;
                if (skipped>D0_BUFFER_LENGTH){
                    _wait_sync_end=false;
                    print(log_error, "stopped searching for wait_sync_end after %d bytes without success!", skipped);
                }
            }
        }
    }
    
	while (::read(_fd, &byte, 1)) {
		lastbyte=byte;
//		if (byte == '/') context = START; 	/* reset to START if "/" reoccurs */
		if ((byte == '/') && (byte_iterator = 0)) context = VENDOR; /* Slash can also be in OBIS String of TD-3511 meter */
		else if (byte == '?' or byte == '!') context = END; /* "!" is the identifier for the END */
//		else if (byte == '!') context = END;	/* "!" is the identifier for the END */
		switch (context) {
			case START:			/* strip the initial "/" */
				if  (byte != '\r' &&  byte != '\n') { /*allow extra new line at the start */
					byte_iterator = number_of_tuples = 0;        /* start */
					context = VENDOR;        /* set new context: START -> VENDOR */
				}
				break;

			case VENDOR:			/* VENDOR has 3 Bytes */
				if  (byte == '\r' or  byte == '\n' or byte == '/' ) {
					byte_iterator = number_of_tuples = 0;
					break;
					}
				/*print(log_debug, "DEBUG Vendor2 byte= %c hex= %x byteIterator= %i ",name().c_str(), byte, byte, byte_iterator);*/

				if (!isalpha(byte)) goto error; /* Vendor ID needs to be alpha */
				vendor[byte_iterator++] = byte;	/* read next byte */
				if (byte_iterator >= 3) {	/* stop after 3rd byte */
					vendor[byte_iterator] = '\0'; /* termination */
					byte_iterator = 0;	/* reset byte counter */

					context = BAUDRATE;	/* set new context: VENDOR -> BAUDRATE */
				} 
				break;

			case BAUDRATE:			/* BAUDRATE consists of 1 char only */
				baudrate = byte;	
				context = IDENTIFICATION;	/* set new context: BAUDRATE -> IDENTIFICATION */
				byte_iterator = 0;
				break;

			case IDENTIFICATION:		/* IDENTIFICATION has 16 bytes */
				if (byte == '\r' || byte == '\n') { /* detect line end */
					identification[byte_iterator] = '\0'; /* termination */
				print(log_debug, "Pull answer (vendor=%s, baudrate=%c, identification=%s)",
							name().c_str(),  vendor, baudrate, identification);
					//context = OBIS_CODE;	/* set new context: IDENTIFICATION -> OBIS_CODE */
					context = ACK;
					byte_iterator = 0;
				}
				else {
					if(!isprint(byte)) {
						print(log_error, "====> binary character '%x'", name().c_str(), byte);
						//error_flag=true;
					}
					else {
						identification[byte_iterator++] = byte;
					}
				//break;
				}
				break;
			case ACK:
				if(_ack.size()) {
					//tcflush(_fd, TCIOFLUSH);
					//usleep (500000);
					if (baudrate_read!=baudrate_connect) {
						cfsetispeed(&tio, baudrate_read);
						tcsetattr(_fd, TCSANOW, &tio);
					}
					int wlen=write(_fd,_ack.c_str(),_ack.size());
					tcdrain(_fd);// Warte bis Ausgang gesendet
					print(log_debug,"sending ack sequenz send (len:%d is:%d,%s).",name().c_str(),_ack.size(),wlen,_ack.c_str());
				}
				context = OBIS_CODE;
				break;
				

			case START_LINE:
				break;

			case OBIS_CODE:
				print(log_debug, "DEBUG OBIS_CODE byte %c hex= %X ",name().c_str(), byte, byte);
				if ((byte != '\n') && (byte != '\r')&& (byte != 0x02))// STX ausklammern 
				{
					if (byte == '(') {
						obis_code[byte_iterator] = '\0';
						byte_iterator = 0;
						context = VALUE;
					}
					else obis_code[byte_iterator++] = byte;
				}
				break;

			case VALUE:
				/*print(log_debug, "DEBUG VALUE byte= %c hex= %x ",name().c_str(), byte, byte);*/
				if (byte == '*' || byte == ')') {
					value[byte_iterator] = '\0';
					byte_iterator = 0;

					if (byte == ')') {
						unit[0] = '\0';
						context =  END_LINE;
					}
					else {
						context = UNIT;
					}
				}
				else value[byte_iterator++] = byte;
				break;

			case UNIT:
				if (byte == ')') {
					unit[byte_iterator] = '\0';
					byte_iterator = 0;

					context = END_LINE;
				}
				else unit[byte_iterator++] = byte;
				break;

			case END_LINE:
				if (byte == '\r' || byte == '\n') {
					/* free slots available and sain content? */
					if ((number_of_tuples < max_readings) && (strlen(obis_code) > 0) && 
							(strlen(value) > 0)) {
						print(log_debug, "Parsed reading (OBIS code=%s, value=%s, unit=%s)", name().c_str(), obis_code, value, unit);
						rds[number_of_tuples].value(strtod(value, NULL));
						if ((obis_code[0]=='1')||(obis_code[0]=='2')||(obis_code[0]=='C')) {
							/*print(log_debug, "DEBUG END_LINE Obis code = %s value %s ",name().c_str(), obis_code, value);*/
							Obis obis(obis_code);
							ReadingIdentifier *rid(new ObisIdentifier(obis));
							rds[number_of_tuples].identifier(rid);
							rds[number_of_tuples].time();
							byte_iterator = 0;
							number_of_tuples++;
						}
					}
					context = OBIS_CODE;
				}
				break;

			case END:
				/*print(log_debug, "DEBUG END1 %c %i ", name().c_str(), byte, byte_iterator);*/
				endseq[byte_iterator++] = byte;
				/*print(log_debug, "DEBUG END2 byte: %c  iterator: %i ", name().c_str(), byte, byte_iterator);*/
				if(endseq[0] == '?' ) {
				/* endseq[byte_iterator++] = byte;*/
				/* context = END; */
				/*print(log_debug, "DEBUG END3 byte: %x endseq: %x ", name().c_str(), byte, endseq);*/
				if(endseq[1] == '!') {
					/*Pullseq /?! was displayed again. Go on with VENDOR*/
					context = VENDOR;
					endseq[byte_iterator] = '\0';
					print(log_debug, "DEBUG END4 goto VENDOR", name().c_str());
					byte_iterator = 0;
					endseq[3] = { 0 };
							}
				break;
						}
				if(error_flag) {
				print(log_error, "reading binary values.", name().c_str());
				goto error;
				}
				print(log_debug, "Read package with %i tuples (vendor=%s, baudrate=%c, identification=%s)",
					name().c_str(), number_of_tuples, vendor, baudrate, identification);
				return number_of_tuples;
		}/* end switch*/
	}/* end while*/
	// Read terminated
	print(log_error, "read timed out!, context: %i, bytes read: %i, last byte 0x%x", name().c_str(),context,byte_iterator,lastbyte);
	return 0;
	error:
	print(log_error, "Something unexpected happened: %s:%i!", name().c_str(), __FUNCTION__, __LINE__);
	return 0;
}

int MeterD0::_openSocket(const char *node, const char *service) {
	struct sockaddr_in sin;
	struct addrinfo *ais;
	int fd; /* file descriptor */
	int res;

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		print(log_error, "socket(): %s", name().c_str(), strerror(errno));
		return ERR;
	}

	getaddrinfo(node, service, NULL, &ais);
	memcpy(&sin, ais->ai_addr, ais->ai_addrlen);
	freeaddrinfo(ais);

	res = connect(fd, (struct sockaddr *) &sin, sizeof(sin));
	if (res < 0) {
		print(log_error, "connect(%s, %s): %s", name().c_str(), node, service, strerror(errno));
		return ERR;
	}

	return fd;
}

int MeterD0::_openDevice(struct termios *old_tio, speed_t baudrate) {
	struct termios tio;
	memset(&tio, 0, sizeof(struct termios));

	int fd = ::open(device(), O_RDWR);
	if (fd < 0) {
		print(log_error, "open(%s): %s", name().c_str(), device(), strerror(errno));
		return ERR;
	}

	/* get old configuration */
	tcgetattr(fd, &tio) ;

	/* backup old configuration to restore it when closing the meter connection */
	memcpy(old_tio, &tio, sizeof(struct termios));
	/* 
	initialize all control characters 
	default values can be found in /usr/include/termios.h, and are given
	in the comments, but we don't need them here
	*/
	tio.c_cc[VINTR]    = 0;     /* Ctrl-c */ 
	tio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
	tio.c_cc[VERASE]   = 0;     /* del */
	tio.c_cc[VKILL]    = 0;     /* @ */
	tio.c_cc[VEOF]     = 4;     /* Ctrl-d */
	tio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
	tio.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
	tio.c_cc[VSWTC]    = 0;     /* '\0' */
	tio.c_cc[VSTART]   = 0;     /* Ctrl-q */ 
	tio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
	tio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
	tio.c_cc[VEOL]     = 0;     /* '\0' */
	tio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
	tio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
	tio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
	tio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
	tio.c_cc[VEOL2]    = 0;     /* '\0' */

	tio.c_iflag &= ~(BRKINT | INLCR | IMAXBEL | IXOFF| IXON);
	tio.c_oflag &= ~(OPOST | ONLCR);
	tio.c_lflag &= ~(ISIG | ICANON | IEXTEN | ECHO);

	switch(_parity) {
	case parity_8n1:
		tio.c_cflag &= ~ PARENB;
		tio.c_cflag &= ~ PARODD;
		tio.c_cflag &= ~ CSTOPB;
		tio.c_cflag &= ~ CSIZE;
		tio.c_cflag |= CS8;
		break;
	case parity_7n1:
		tio.c_cflag &= ~ PARENB;
		tio.c_cflag &= ~ PARODD;
		tio.c_cflag &= ~ CSTOPB;
		tio.c_cflag &= ~ CSIZE;
		tio.c_cflag |= CS7;
		break;
	case parity_7e1:
		tio.c_cflag &= ~CRTSCTS;
		tio.c_cflag |=  PARENB;
		tio.c_cflag &= ~ PARODD;
		tio.c_cflag &= ~ CSTOPB;
		tio.c_cflag &= ~ CSIZE;
		tio.c_cflag |= CS7;
		break;
	case parity_7o1:
		tio.c_cflag &= ~ PARENB;
		tio.c_cflag |=  PARODD;
		tio.c_cflag &= ~ CSTOPB;
		tio.c_cflag &= ~ CSIZE;
		tio.c_cflag |= CS7;
		break;
	}
/* Set return rules for read to prevent endless waiting*/
 	tio.c_cc[VTIME]    = 50;     /* inter-character timer  50*0.1s*/
        tio.c_cc[VMIN]     = 0;     /* VTIME is timeout counter */
       /* 
          now clean the modem line and activate the settings for the port
        */
        tcflush(fd, TCIOFLUSH);
	/* set baudrate */
	cfsetispeed(&tio, baudrate);
	cfsetospeed(&tio, baudrate);

	/* apply new configuration */
	tcsetattr(fd, TCSANOW, &tio);

	return fd;
}
