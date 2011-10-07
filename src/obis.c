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

#include "obis.h"

obis_alias_t obis_aliases[] = {
/**
 * 255 is considered as wildcard!
 *
 *   A    B    C    D    E    F    alias	description
 * ===================================================================================*/

/* General */
{{{  1,   0,  12,   7,   0, 255}}, "voltage",	""},
{{{  1,   0,  11,   7,   0, 255}}, "current",	""},
{{{  1,   0,  14,   7,   0, 255}}, "frequency",	""},
{{{  1,   0,  12,   7,   0, 255}}, "powerfactor",""},

{{{  1,   0,   1,   7, 255, 255}}, "power",	"Active Power Instantaneous value Total"},
{{{  1,   0,  21,   7, 255, 255}}, "power-l1",	"L1 Active Power Instantaneous value Total"},
{{{  1,   0,  41,   7, 255, 255}}, "power-l2",	"L1 Active Power Instantaneous value Total"},
{{{  1,   0,  61,   7, 255, 255}}, "power-l3",	"L3 Active Power Instantaneous value Total"},

{{{  1,   0,   1,   8,	0, 255}}, "counter",	"Active Power Counter Total"},

/* Easymeter */
{{{  1,   0,  96,   5,   5, 255}}, "status",	"Meter status flag"},

/* ESYQ3B (Easymeter Q3B) */
{{{129, 129, 199, 130,   3, 255}}, "",		""}, // ???
{{{  1,   0,   1,   8,   1, 255}}, "counter-t1",	"Active Power Counter Tariff 1"},
{{{  1,   0,   1,   8,   2, 255}}, "counter-t2",	"Active Power Counter Tariff 2"},

/* ESYQ3D (Easymeter Q3D) */
{{{  0,   0,  96,   1, 255, 255}}, "device",	"Complete device ID"},
{{{  0,   0,   0,   0,   0, 255}}, "",		""}, // ???

{} /* stop condition for iterator */
};

obis_id_t obis_init(const unsigned char *raw) {
	obis_id_t id;
	
	if (raw == NULL) {
		memset(id.raw, 0, 6); /* initialize with zeros */
	}
	else {
		memcpy(id.raw, raw, 6);
	}
	
	return id;
}

obis_id_t obis_parse(const char *str) {
	obis_id_t id;
	regex_t re;
	regmatch_t matches[7];

	regcomp(&re, "^([0-9])-([0-9]{,2}):([0-9]{,2})\\.([0-9]{,2})\\.([0-9]{,2})(\\[*&][0-9]{,2})?$", REG_EXTENDED | REG_ICASE);
	// TODO make values A B C optional to allow notations like "1.8.0"

	if (regexec(&re, str, 7, matches, 0) == 0) { /* found string in OBIS notation */
		for (int i=0; i<6; i++) {
			if (matches[i+1].rm_so != -1) {
				id.raw[i] = strtoul(str+matches[i+1].rm_so, NULL, 10);
			}
			else {
				id.raw[i] = 0xff; /* default value */
			}
		}
	}
	else { /* looking for alias */
		id = obis_lookup_alias(str);
	}
	
	regfree(&re); /* householding */

	return id;
}

obis_id_t obis_lookup_alias(const char *alias) {
	obis_id_t nf = obis_init(NULL); /* not found */
	obis_alias_t *it = obis_aliases;

	do { /* linear search */
		if (strcmp(it->name, alias) == 0) {
			return it->id;
		}
	} while ((++it)->name);
	
	return nf;
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

int obis_compare(obis_id_t a, obis_id_t b) {
	for (int i = 0; i < 6; i++) {
		if (a.raw[i] == b.raw[i] || a.raw[i] == 255 || b.raw[i] == 255 ) {
			continue; /* skip on wildcard or equal */
		}
		else if (a.raw[i] < b.raw[i]) {
			return -1;
		}
		else if (a.raw[i] > b.raw[i]) {
			return 1;
		}
	}
	
	return 0; /* equal */
}

int obis_is_null(obis_id_t id) {
	return !(
		id.raw[0] ||
		id.raw[1] ||
		id.raw[2] ||
		id.raw[3] ||
		id.raw[4] ||
		id.raw[5]
	);
}

int obis_is_manufacturer_specific(obis_id_t id) {
	return (
		(id.groups.channel >= 128 && id.groups.channel <= 199) ||
		(id.groups.indicator >= 128 && id.groups.indicator <= 199) ||
		(id.groups.indicator == 240) ||
		(id.groups.mode >= 128 && id.groups.mode <= 254) ||
		(id.groups.quantities >= 128 && id.groups.quantities <= 254) ||
		(id.groups.storage >= 128 && id.groups.storage <= 254)
	);
}


