/**
 * Protocol interface
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

#include <string.h>
#include <stdio.h>

#include "Meter.hpp"
#include "Options.hpp"
#include <common.h>
#include <VZException.hpp>
#include <protocols/MeterD0.hpp>
#include <protocols/MeterExec.hpp>
#include <protocols/MeterFile.hpp>
#include <protocols/MeterFluksoV2.hpp>
#include <protocols/MeterRandom.hpp>
#include <protocols/MeterS0.hpp>
#ifdef SML_SUPPORT
#include <protocols/MeterSML.hpp>
#endif
#ifdef OCR_SUPPORT
#include "protocols/MeterOCR.hpp"
#endif
//#include <protocols/.h>

#define METER_DETAIL(NAME, CLASSNAME, DESC, MAX_RDS, PERIODIC) {				\
		meter_protocol_##NAME, #NAME, DESC, MAX_RDS, PERIODIC/*, Meter##CLASSNAME */}

int Meter::instances=0;

static const meter_details_t protocols[] = {
/*  aliasdescriptionmax_rdsperiodic
	===============================================================================================*/
	METER_DETAIL(file, File, "Read from file or fifo",32,true),
	// METER_DETAIL(exec, "Parse program output",32,true),
	METER_DETAIL(random, Random, "Generate random values with a random walk",1,true),
	METER_DETAIL(fluksov2, Fluksov2, "Read from Flukso's onboard SPI fifo",16,false),
	METER_DETAIL(s0, S0, "S0-meter directly connected to RS232",2,false),
	METER_DETAIL(d0, D0, "DLMS/IEC 62056-21 plaintext protocol",400,false),
#ifdef SML_SUPPORT
	METER_DETAIL(sml, Sml, "Smart Message Language as used by EDL-21, eHz and SyM²", 32,false),
#endif // SML_SUPPORT
#ifdef OCR_SUPPORT
	METER_DETAIL(ocr, OCR, "Image processing/recognizing meter", 32, false), // TODO periodic or not periodic?
#endif
//{} /* stop condition for iterator */
	METER_DETAIL(none, NULL,NULL, 0,false),
};

Meter::Meter(std::list<Option> pOptions) :
		_name("meter")
{
	id = instances++;
	OptionList optlist;

	// set meter name
	std::stringstream oss;
	oss<<"mtr"<< id;
	_name = oss.str();

	//optlist.dump(pOptions);

	try {
		// protocol
		const char *protocol_str = optlist.lookup_string(pOptions, "protocol");
		print(log_debug, "Creating new meter with protocol %s.", name(), protocol_str);

		if (meter_lookup_protocol(protocol_str, &_protocol_id) != SUCCESS) {
//print(log_error, "Invalid protocol: %s", mtr, protocol_str);
//return ERR; /* skipping this meter */
			throw vz::VZException("Protocol not found.");
		}
	} catch (vz::VZException &e) {
		std::stringstream oss;
		oss << e.what();
		print(log_error, "Missing protocol or invalid type (%s)", name(), oss.str().c_str());
		throw;
	}

	try {
		// interval
		Option interval_opt = optlist.lookup(pOptions, "interval");
		_interval = (int)(interval_opt);
	} catch (vz::OptionNotFoundException &e) {
		_interval = -1; /* indicates unknown interval */
	} catch (vz::VZException &e) {
		print(log_error, "Invalid type for interval", name());
		throw;
	}
	try {
		// aggregation time
		Option interval_opt = optlist.lookup(pOptions, "aggtime");
		_aggtime = (int)(interval_opt);
	} catch (vz::OptionNotFoundException &e) {
		_aggtime = -1; /* indicates no aggregation */
	} catch (vz::VZException &e) {
		print(log_error, "Invalid type for aggtime", name());
		throw;
	}
	try {
		_aggFixedInterval = optlist.lookup_bool(pOptions, "aggfixedinterval");
	} catch (vz::OptionNotFoundException &e) {
		_aggFixedInterval = false;
	} catch (vz::VZException &e) {
		print(log_error, "Invalid type for aggfixedinterval", name());
		throw;
	}

	try{
		const meter_details_t *details = meter_get_details(_protocol_id);
		if (details->periodic == true && _interval < 0) {
			print(log_error, "Interval has to be set and positive!", name());
		}
	} catch (vz::VZException &e) {
		std::stringstream oss;
		oss << e.what();
		print(log_error, "Missing protocol or invalid type (%s)", name(), oss.str().c_str());
		throw;
	}
	switch(_protocol_id) {
		case meter_protocol_file:
			_protocol = vz::protocol::Protocol::Ptr(new MeterFile(pOptions));
			_identifier = ReadingIdentifier::Ptr(new StringIdentifier());
			break;
		case meter_protocol_exec:
			_protocol = vz::protocol::Protocol::Ptr(new MeterExec(pOptions));
			_identifier = ReadingIdentifier::Ptr(new StringIdentifier());
			break;
		case meter_protocol_random:
			_protocol = vz::protocol::Protocol::Ptr(new MeterRandom(pOptions));
			_identifier = ReadingIdentifier::Ptr(new NilIdentifier());
			break;
		case meter_protocol_s0:
			_protocol = vz::protocol::Protocol::Ptr(new MeterS0(pOptions));
			_identifier = ReadingIdentifier::Ptr(new StringIdentifier());
			break;
		case meter_protocol_d0:
			_protocol = vz::protocol::Protocol::Ptr(new MeterD0(pOptions));
			_identifier = ReadingIdentifier::Ptr(new ObisIdentifier());
			break;
#ifdef SML_SUPPORT
		case  meter_protocol_sml:
			_protocol = vz::protocol::Protocol::Ptr(new MeterSML(pOptions));
			_identifier = ReadingIdentifier::Ptr(new ObisIdentifier());
			break;
#endif
		case meter_protocol_fluksov2:
			_protocol = vz::protocol::Protocol::Ptr(new MeterFluksoV2(pOptions));
			_identifier = ReadingIdentifier::Ptr(new ChannelIdentifier());
			break;
#ifdef OCR_SUPPORT
		case meter_protocol_ocr:
			_protocol = vz::protocol::Protocol::Ptr(new MeterOCR(pOptions));
			_identifier = ReadingIdentifier::Ptr(new StringIdentifier());
			break;
#endif
		default:
			break;
	}

	try {
		// enable
		_enable = optlist.lookup_bool(pOptions, "enabled");
	} catch (vz::OptionNotFoundException &e) {
		_enable = false; /* bye default meter is disabled */
	} catch (vz::VZException &e) {
		print(log_error, "Invalid type for enable", name());
		throw;
	}

	try {
		// skip
		_skip = optlist.lookup_bool(pOptions, "allowskip");
	} catch (vz::OptionNotFoundException &e) {
		_skip = false; /* by default meter will throw an exception if opening fails */
	} catch (vz::VZException &e) {
		print(log_error, "Invalid type for allowskip", name());
		throw;
	}

	print(log_debug, "Meter configured, %s.", name(), _enable ? "enabled" : "disabled");
}

//Meter::Meter(const Meter *mtr) {
//}

Meter::~Meter() {
}

void Meter::open() {
	if (_protocol->open() < 0) {
		print(log_error, "Cannot open meter", name());
		throw vz::ConnectionException("Meter open failed.");
	}
}

int Meter::close() {
	return _protocol->close();
}

size_t Meter::read(std::vector<Reading> &rds, size_t n) {
	return _protocol->read(rds, n);
}

int meter_lookup_protocol(const char* name, meter_protocol_t *protocol) {
	if (!name) return ERR_NOT_FOUND;
	for (const meter_details_t *it = meter_get_protocols(); it->id != meter_protocol_none; it++) { // we have to stop when the id is null not the ptr to the array!
		if (it->name && (strcmp(it->name, name) == 0)) {
			if (protocol)
				*protocol = it->id; // else ignore anyhow. can be used to check whether a protocol exists.
			return SUCCESS;
		}
	}
	return ERR_NOT_FOUND;
}

const meter_details_t * meter_get_protocols() {
	return protocols;
}

const meter_details_t * meter_get_details(meter_protocol_t protocol) {
	for (const meter_details_t *it = protocols; it->id != meter_protocol_none; it++) {
		if (it->id == protocol) {
			return it;
		}
	}
	return NULL;
}
