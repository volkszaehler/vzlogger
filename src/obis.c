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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "../include/obis.h"

obis_alias_t obis_aliases[] = {
//   A   B   C   D   E   F     abbreviation	description
//=====================================================================================
{{"\x81\x81\xC7\x82\x03\xFF"}, "voltage",	""},
{{"\x81\x81\xC7\x82\x03\xFF"}, "current",	""},
{{"\x81\x81\xC7\x82\x03\xFF"}, "frequency",	""},
{{"\x81\x81\xC7\x82\x03\xFF"}, "powerfactor",	""},

/* ESYQ3B (Easymeter Q3B) */
{{"\x81\x81\xC7\x82\x03\xFF"}, "vendor",	"vendor specific"},
{{"\x01\x00\x01\x08\x00\xFF"}, "counter",	"Active Power Counter Total"},
{{"\x01\x00\x01\x08\x01\xFF"}, "counter-tarif1","Active Power Counter Tariff 1"},
{{"\x01\x00\x01\x08\x02\xFF"}, "counter-tarif2","Active Power Counter Tariff 2"},
{{"\x01\x00\x01\x07\x00\xFF"}, "power",		"Active Power Instantaneous value Total"},
{{"\x01\x00\x15\x07\x00\xFF"}, "power-l1",	"L1 Active Power Instantaneous value Total"},
{{"\x01\x00\x29\x07\x00\xFF"}, "power-l2",	"L1 Active Power Instantaneous value Total"},
{{"\x01\x00\x3D\x07\x00\xFF"}, "power-l2",	"L3 Active Power Instantaneous value Total"},
{{"\x01\x00\x60\x05\x05\xFF"}, "status",	"Meter status flag"},
{} /* stop condition for iterator */
};

obis_id_t obis_init(unsigned char *raw) {
	obis_id_t id;
	memcpy(id.raw, raw, 6);
	return id;
}

obis_id_t obis_parse(char *str) {
	obis_id_t id;
	regex_t expr;
	regmatch_t matches[7];

	regcomp(&expr, "^([0-9])-([a-f0-9]{,2}):([a-f0-9]{,2})\\.([a-f0-9]{,2})\\.([a-f0-9]{,2})(\\*[a-f0-9]{,2})?$", REG_EXTENDED | REG_ICASE);

	if (regexec(&expr, str, 7, matches, 0) == 0) { /* found string in OBIS notation */
		for (int i=0; i<6; i++) {
			if (matches[i+1].rm_so != -1) {
				id.raw[i] = strtoul(str+matches[i+1].rm_so, NULL, 16);
			}
			else {
				id.raw[i] = 0xff; /* default value */
			}
		}
	}
	else { /* looking for alias */
		obis_alias_t *it = obis_aliases;
		do { /* linear search */
			if (strcmp(it->name, str) == 0) {
				return it->id;
			}
		} while ((++it)->name);
	}

	return id;
}

char obis_is_manufacturer_specific(obis_id_t id) {
	return (
		(id.groups.channel >= 128 && id.groups.channel <= 199) ||
		(id.groups.indicator >= 128 && id.groups.indicator <= 199) ||
		(id.groups.indicator == 240) ||
		(id.groups.mode >= 128 && id.groups.mode <= 254) ||
		(id.groups.quantities >= 128 && id.groups.quantities <= 254) ||
		(id.groups.storage >= 128 && id.groups.storage <= 254)
	);
}

int obis_unparse(obis_id_t id, char *buffer) {
	return sprintf(buffer, "%x-%x:%x.%x.%x*%x",
		id.groups.media,
		id.groups.channel,
		id.groups.indicator,
		id.groups.mode,
		id.groups.quantities,
		id.groups.storage
	);
}

