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

#ifndef _READING_H_
#define _READING_H_

#include <sys/time.h>

#include "obis.h"

#define MAX_IDENTIFIER_LEN 255

/* Identifiers */
class ReadingIdentifier {
public:
	virtual bool operator==(ReadingIdentifier &cmp) = 0;
};

class ObisIdentifier : public ReadingIdentifier {
public:
	bool operator==(ObisIdentifier &cmp);
protected:
	obis_id_t obis;
};

class StringIdentifier : public ReadingIdentifier {
public:
	bool operator==(StringIdentifier &cmp);
protected:
	char *string;
};

class UuidIdentifier : public ReadingIdentifier {
public:
	bool operator==(UuidIdentifier &cmp);
protected:
	char *uuid;
};

class ChannelIdentifier : public ReadingIdentifier {
public:
	bool operator==(ChannelIdentifier &cmp);
protected:
	int channel;
};


class Reading {

public:
	Reading(double pValue, struct timeval pTime, ReadingIdentifier pIndentifier);

	static double tvtod(struct timeval tv);
	static struct timeval dtotv(double ts);

protected:
	double value;
	struct timeval time;
	ReadingIdentifier identifier;
};

enum meter_procotol; /* forward declaration */

/**
 * Parse identifier by a given string and protocol
 *
 * @param protocol the given protocol context in which the string should be parsed
 * @param string the string-encoded identifier
 * @return 0 on success, < 0 on error
 */
int reading_id_parse(enum meter_procotol protocol, reading_id_t *id, const char *string);

/**
 * Print identifier to buffer for debugging/dump
 *
 * @return the amount of bytes used in buffer
 */
size_t reading_id_unparse(enum meter_procotol protocol, reading_id_t identifier, char *buffer, size_t n);

#endif /* _READING_H_ */
