/**
 * OBIS IDs as specified in DIN EN 62056-61
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
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

#include <string>

#define OBIS_STR_LEN (6 * 3 + 5 + 1)

class Obis {
  public:
	Obis(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e,
		 unsigned char f);
	Obis(const char *strClear); /* attention: no hex codes inside the strings! Only special
								   characters "C, F, L, P" allowed. */
	Obis();                     // initializes to "not given" = all DC/255.

	// static Obis lookup(const char *alias);

	/* regex: A-BB:CC.DD.EE([*&]FF)? */
	size_t unparse(char *buffer, size_t n);
	const std::string toString();

	bool operator==(const Obis &rhs) const;

	bool isManufacturerSpecific() const;
	bool isAllNotGiven() const; // check whether all are not given (=DC/255)
	bool isValid() const;

  private:
	int parse(const char *str);
	int lookup_alias(const char *alias);

  private:
	union {
		unsigned char _raw[6];
		struct {
			unsigned char media, channel, indicator, mode, quantities;
			unsigned char storage; /* not used in Germany */
		} groups;
	} _obisId;
};

typedef struct {
	Obis id;
	const char *name;
	const char *desc;
} obis_alias_t;

obis_alias_t *obis_get_aliases();

#endif /* _OBIS_H_ */
