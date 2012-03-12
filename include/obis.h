/**
 * OBIS IDs as specified in DIN EN 62056-61
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
 
#ifndef _OBIS_H_
#define _OBIS_H_

#include "reading.h"

#define OBIS_STR_LEN (6*3+5+1)

class Obis {

public:
	Obis(unsigned char *pRaw = NULL);

	static Obis lookup(const char *alias);

	/* regex: A-BB:CC.DD.EE([*&]FF)? */
	void parse(const char *str);
	void unparse(char *buffer, size_t n) const;

	bool operator==(Obis const &cmp) const;

	bool isManufacturerSpecific() const;
	bool isNull() const;

protected:
	unsigned char raw[6];

	typedef struct {
		unsigned char raw[6];
		char *name;
		char *desc;
	} ObisAlias;

	static ObisAlias aliases[];
};

class ObisIdentifier : public ReadingIdentifier, public Obis {

public:
	bool operator==(ObisIdentifier &cmp);
	void parse(const char *string);
	size_t unparse(char *buffer, size_t n);
};

#endif /* _OBIS_H_ */
