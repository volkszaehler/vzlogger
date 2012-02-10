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

#define OBIS_STR_LEN (6*3+5+1)

/* regex: A-BB:CC.DD.EE([*&]FF)? */
class Obis {

public:
	Obis(unsigned char *pRaw);

	static Obis getByAlias(const char *alias);

	void parse(const char *str);
	void unparse(char *buffer, size_t n);

	bool operator==(Obis &cmp);

	bool isManufacturerSpecific() const;
	bool isNull() const;

protected:
	unsigned char raw[6];
}

typedef struct {
	obis_id_t id;
	char *name;
	char *desc;
} obis_alias_t;

#endif /* _OBIS_H_ */
