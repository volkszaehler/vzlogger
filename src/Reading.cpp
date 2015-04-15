/**
 * Reading related functions
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

#include <iostream>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "VZException.hpp"
#include "Reading.hpp"

Reading::Reading()
		: _deleted(false)
		, _value(0)
{
	_time.tv_sec = 0;
	_time.tv_usec = 0;
}

Reading::Reading(ReadingIdentifier::Ptr pIndentifier)
		: _deleted(false)
		, _value(0)
		, _identifier(pIndentifier)
{
	_time.tv_sec = 0;
	_time.tv_usec = 0;
}
Reading::Reading(
	double pValue
	, struct timeval pTime
	, ReadingIdentifier::Ptr pIndentifier
	)
		: _deleted(false)
		, _value(pValue)
		, _time(pTime)
		, _identifier(pIndentifier)
{
}

Reading::Reading(
	const Reading &orig
	) :
		_deleted(orig._deleted)
		, _value(orig._value)
		, _time(orig._time)
		, _identifier (orig._identifier)
{
}

void Reading::time_from_double(double const &ts)
{
	double integral;
	double fraction = modf(ts, &integral);

	_time.tv_usec = (long int) (fraction * 1e6);
	_time.tv_sec = (long int) integral;
}

ReadingIdentifier::Ptr reading_id_parse(meter_protocol_t protocol, const char *string) {
	ReadingIdentifier::Ptr rid;

	switch (protocol) {
			case meter_protocol_d0:
			case meter_protocol_sml:
				rid = ReadingIdentifier::Ptr(new ObisIdentifier(Obis(string)));
				break;

			case meter_protocol_fluksov2: {
				char type[13];
				int channel;

				int ret = sscanf(string, "sensor%u/%12s", &channel, type);
				if (ret != 2) {
					throw vz::VZException("meter-fluksov4 failed");
				}
				rid = ReadingIdentifier::Ptr(new ChannelIdentifier(channel+1));

				//id->channel = channel + 1; /* increment by 1 to distinguish between +0 and -0 */

#if 0
				if (strcmp(type, "consumption") == 0) {
					id->channel *= -1;
				}
				else if (strcmp(type, "power") != 0) {
					throw std::exception("type not found");
					//return ERR;
				}
#endif
				break;
			}

			case meter_protocol_file:
			case meter_protocol_exec:
			case meter_protocol_s0:
			case meter_protocol_ocr:
				rid = ReadingIdentifier::Ptr(new StringIdentifier(string));
				break;

			default: /* ignore other protocols which do not provide id's */
				rid = ReadingIdentifier::Ptr(new NilIdentifier());
				break;
	}

	return rid;
}

// reading_id_unparse
size_t Reading::unparse(
//	meter_protocol_t protocol,
	char *buffer, size_t n
	) {

	return _identifier->unparse(buffer, n);

#if 0
	switch (protocol) {
			case meter_protocol_d0:
			case meter_protocol_sml:
				//obis_unparse(id.obis, buffer, n);
				break;

			case meter_protocol_fluksov2:
				//snprintf(buffer, n, "sensor%u/%s", abs(id.channel) - 1, (id.channel > 0) ? "power" : "consumption");
				break;

			case meter_protocol_file:
			case meter_protocol_exec:
				//if (id.string != NULL) {
//		strncpy(buffer, id.string, n);
//		break;
				//}

			default:
				buffer[0] = '\0';
	}

	return strlen(buffer);
#endif
}

bool ReadingIdentifier::operator==( ReadingIdentifier const &cmp) const {
	return this->compare(this, &cmp);
}

bool ReadingIdentifier::compare( ReadingIdentifier const *lhs,  ReadingIdentifier const *rhs) const {
	if (ObisIdentifier const* lhsx = dynamic_cast<ObisIdentifier const*>(lhs)) {
		if (ObisIdentifier const* rhsx = dynamic_cast<ObisIdentifier const*>(rhs)) {
			return *lhsx == *rhsx;
		} else { return false; }
	} else
		if (StringIdentifier const* lhsx = dynamic_cast<StringIdentifier const*>(lhs)) {
			if (StringIdentifier const* rhsx = dynamic_cast<StringIdentifier const*>(rhs)) {
				return *lhsx == *rhsx;
			} else { return false; }
		} else
			if (ChannelIdentifier const* lhsx = dynamic_cast<ChannelIdentifier const*>(lhs)) {
				if (ChannelIdentifier const* rhsx = dynamic_cast<ChannelIdentifier const*>(rhs)) {
					return *lhsx == *rhsx;
				} else { return false; }
			}  else
				if (NilIdentifier const* lhsx = dynamic_cast<NilIdentifier const*>(lhs)) {
					if (NilIdentifier const* rhsx = dynamic_cast<NilIdentifier const*>(rhs)) {
						(void)lhsx; (void) rhsx; // avoid compiler warning about unused vars.
						return true;
					} else { return false; }
				}

	return false;
}

size_t ObisIdentifier::unparse(char *buffer, size_t n) {
	return _obis.unparse(buffer, n);
}
bool ObisIdentifier::operator==(ObisIdentifier const &cmp) const {
	return (_obis == cmp.obis());
}

/* StringIdentifier */
bool StringIdentifier::operator==(StringIdentifier const &cmp) const{
	return (_string == cmp._string);
}

void StringIdentifier::parse(const char *string) {
	_string = string;
}

size_t StringIdentifier::unparse(char *buffer, size_t n) {
	if (_string != "") {
		strncpy(buffer, _string.c_str(), n);
		if (n>0) buffer[n-1] = '\0';
	} else {
		strncpy(buffer, "", n);
	}

	return strlen(buffer);
}

/* ChannelIdentifier */
bool ChannelIdentifier::operator==(ChannelIdentifier const &cmp) const {
	return (_channel == cmp._channel);
}

void ChannelIdentifier::parse(const char *string) {
	char type[13];
	int channel;
	int ret = sscanf(string, "sensor%u/%12s", &channel, type);

	if (ret != 2) {
		throw vz::VZException("Failed to parse channel identifier");
	}

	_channel = channel + 1; /* increment by 1 to distinguish between +0 and -0 */

	if (strcmp(type, "consumption") == 0) {
		_channel *= -1;
	}
	else if (strcmp(type, "power") != 0) {
		throw vz::VZException("Invalid channel type");
	}
}

size_t ChannelIdentifier::unparse(char *buffer, size_t n) {
	return snprintf(buffer, n, "sensor%u/%s", abs(_channel) - 1, (_channel > 0) ? "power" : "consumption");
}

size_t NilIdentifier::unparse(char *buffer, size_t n) {
	return snprintf(buffer, n, "NilIdentifier");
//buffer[0] = '\0';
	//return strlen(buffer);
}
