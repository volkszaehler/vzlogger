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

#include <string>

#define OBIS_STR_LEN (6*3+5+1)

/* regex: A-BB:CC.DD.EE([*&]FF)? */
class Obis {
  public:
	Obis(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e, unsigned char f);
	Obis(unsigned char *pRaw);
	Obis(){};

	//static Obis getByAlias(const char *alias);

	int parse(const char *str);
	size_t unparse(char *buffer, size_t n);

	const bool operator==(const Obis &rhs);

	const bool isManufacturerSpecific() const;
  const bool isNull() const;

  protected:
  union {
    unsigned char _raw[6];
    struct {
      unsigned char media, channel, indicator, mode, quantities;
      unsigned char storage;	/* not used in Germany */
    } groups;
  } _obisId;
};

/*
class Obis_Alias {
public:
  Obis_Alias(){}

private:
	//Obis _id;
	char *_name;
	char *_desc;
};
*/

typedef struct {
//  public:
//	Obis &id() { return &_id; }

  Obis id;
	const char *name;
	const char *desc;
} obis_alias_t;

obis_alias_t * obis_get_aliases();

#endif /* _OBIS_H_ */
