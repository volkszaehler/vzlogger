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
#include <ctype.h>

#include "obis.h"
#include "common.h"

#define DC 0xff // wildcard, dont care

static const obis_alias_t aliases[] = {
/*   A    B    C    D    E    F    alias		description
====================================================================*/

/* general */
{{{  1,   0,   1,   7,  DC,  DC}}, "power",		"Wirkleistung  (Summe)"},
{{{  1,   0,  21,   7,  DC,  DC}}, "power-l1",		"Wirkleistung  (Phase 1)"},
{{{  1,   0,  41,   7,  DC,  DC}}, "power-l2",		"Wirkleistung  (Phase 2)"},
{{{  1,   0,  61,   7,  DC,  DC}}, "power-l3",		"Wirkleistung  (Phase 3)"},

{{{  1,   0,  12,   7,  DC,  DC}}, "voltage",		"Spannung      (Mittelwert)"},
{{{  1,   0,  32,   7,  DC,  DC}}, "voltage-l1",	"Spannung      (Phase 1)"},
{{{  1,   0,  52,   7,  DC,  DC}}, "voltage-l2",	"Spannung      (Phase 2)"},
{{{  1,   0,  72,   7,  DC,  DC}}, "voltage-l3",	"Spannung      (Phase 3)"},

{{{  1,   0,  11,   7,  DC,  DC}}, "current",		"Stromstaerke  (Summe)"},
{{{  1,   0,  31,   7,  DC,  DC}}, "current-l1",	"Stromstaerke  (Phase 1)"},
{{{  1,   0,  51,   7,  DC,  DC}}, "current-l2",	"Stromstaerke  (Phase 2)"},
{{{  1,   0,  71,   7,  DC,  DC}}, "current-l3",	"Stromstaerke  (Phase 3)"},

{{{  1,   0,  14,   7,   0,  DC}}, "frequency",		"Netzfrequenz"},
{{{  1,   0,  12,   7,   0,  DC}}, "powerfactor",	"Leistungsfaktor"},

{{{  0,   0,  96,   1,  DC,  DC}}, "device",		"Zaehler Seriennr."},
{{{  1,   0,  96,   5,   5,  DC}}, "status",		"Zaehler Status"},

{{{  1,   0,   1,   8,	DC, DC}}, "counter",		"Zaehlerstand Wirkleistung"},
{{{  1,   0,   2,   8,  DC, DC}}, "counter-out",	"Zaehlerstand Lieferg."},

/* Easymeter */

/* ESYQ3B (Easymeter Q3B) */
{{{  1,   0,   1,   8,   1,  DC}}, "esy-counter-t1",	"Active Power Counter Tariff 1"},
{{{  1,   0,   1,   8,   2,  DC}}, "esy-counter-t2",	"Active Power Counter Tariff 2"},
//{{{129, 129, 199, 130,   3,  DC}}, "",		""}, // ???

/* ESYQ3D (Easymeter Q3D) */
//{{{  0,   0,   0,   0,   0,  DC}}, "",		""}, // ???

/* HAG eHZ010C_EHZ1WA02 (Hager eHz) */
{{{  1,   0,   0,   0,   0,  DC}}, "hag-id",		"Eigentumsnr."},
{{{  1,   0,  96,  50,   0,   0}}, "hag-status",	"Netz Status"},			/* bitcodiert: Drehfeld, Anlaufschwelle, Energierichtung */
{{{  1,   0,  96,  50,   0,   1}}, "hag-frequency",	"Netz Periode"},		/* hexadezimal (Einheit 1/100 ms) */
{{{  1,   0,  96,  50,   0,   2}}, "hag-temp",		"aktuelle Chiptemperatur"},	/* hexadezimal, Einheit °C */
{{{  1,   0,  96,  50,   0,   3}}, "hag-temp-min",	"minimale Chiptemperatur"},
{{{  1,   0,  96,  50,   0,   4}}, "hag-temp-avg",	"gemittelte Chiptemperatur"},
{{{  1,   0,  96,  50,   0,   5}}, "hag-temp-max",	"maximale Chiptemperatur"},
{{{  1,   0,  96,  50,   0,   6}}, "hag-check",		"Kontrollnr."},
{{{  1,   0,  96,  50,   0,   7}}, "hag-diag",		"Diagnose"},

{} /* stop condition for iterator */
};


const obis_alias_t * obis_get_aliases() {
	return aliases;
}

obis_id_t * obis_init(obis_id_t *id, unsigned char *raw) {
	if (raw == NULL) {
		memset(id->raw, 0, 6); /* initialize with zeros */
	}
	else {
		memcpy(id->raw, raw, 6);
	}

	return id;
}

int obis_parse(const char *str, obis_id_t *id) {
	enum { A = 0, B, C, D, E, F };

	char b = 0;
	int num = 0;
	int field = -1;
	size_t n = strlen(str);

	memset(&id->raw, DC, 6); /* initialize as wildcard */

	/* format: "A-B:C.D.E[*&]F" */
	/* fields A, B, E, F are optional */
	/* fields C & D are mandatory */
	for (int i = 0; i < n; i++) {
		b = str[i];

		if (isdigit(b)) {
			num = (num * 10) + (b - '0'); /* parse digits */
		}
		else {
			if (b == '-' && field < A) {		/* end of field A */
				field = A;
			}
			else if (b == ':' && field < B) {	/* end of field B */
				field = B;
			}
			else if (b == '.' && field < D) {	/* end of field C & D*/
				field = (field < C) ? C : D;
			}
			else if ((b == '*' || b == '&') && field == D) { /* end of field E, start of field F */
				field = E;
			}
			else {
				return ERR;
			}

			id->raw[field] = num;
			num = 0;
		}
	}

	/* set last field */
	id->raw[++field] = num;

	/* fields C & D are mandatory */
	return (field < D) ? ERR : SUCCESS;
}

int obis_lookup_alias(const char *alias, obis_id_t *id) {
	for (const obis_alias_t *it = aliases; it != NULL; it++) {
		if (strcmp(it->name, alias) == 0) {
			*id = it->id;
			return SUCCESS;
		}
	}
	return ERR_NOT_FOUND;
}

int obis_unparse(obis_id_t id, char *buffer, size_t n) {
	return snprintf(buffer, n, "%i-%i:%i.%i.%i*%i",
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
		if (a.raw[i] == b.raw[i] || a.raw[i] == 0xff || b.raw[i] == 0xff ) {
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

