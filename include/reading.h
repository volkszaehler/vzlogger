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

#define MAX_IDENTIFIER_LEN 255

/* Identifiers */
class ReadingIdentifier {

public:
	virtual bool operator==(ReadingIdentifier &cmp) = 0;
	virtual void parse(const char *string) = 0;
	virtual size_t unparse(char *buffer, size_t n) = 0;
};

class StringIdentifier : public ReadingIdentifier {

public:
	bool operator==(ReadingIdentifier &cmp);
	void parse(const char *string);
	size_t unparse(char *buffer, size_t n);

	~StringIdentifier();

protected:
	char *string;
};

class ChannelIdentifier : public ReadingIdentifier {

public:
	bool operator==(ReadingIdentifier &cmp);
	void parse(const char *string);
	size_t unparse(char *buffer, size_t n);

protected:
	int channel;
};

class Reading {

public:
	static double tvtod(struct timeval tv);
	static struct timeval dtotv(double ts);

	double value;
	struct timeval time;
	ReadingIdentifier *identifier;
};

#endif /* _READING_H_ */
