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

#include <iostream>
#include <sstream>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Obis.hpp"
#include "common.h"
#include <VZException.hpp>

#define DC 0xff // not specified/not given
#define SC_C                                                                                       \
	96 // special character "C" has obis code 96 according to
	   // http://www.mayor.de/lian98/doc.de/pdf/vdew-lh-lastgangzaehler-2127b3.pdf
#define SC_F 97
#define SC_L 98
#define SC_P 99

// const Obis::aliases[] = {
static obis_alias_t aliases[] = {
	//	A    B    C    D    E    F    alias		description
	//	====================================================================

	//	General

	{Obis(1, 0, 1, 7, DC, DC), "power", "Wirkleistung  (Summe)"},
	{Obis(1, 0, 21, 7, DC, DC), "power-l1", "Wirkleistung  (Phase 1)"},
	{Obis(1, 0, 41, 7, DC, DC), "power-l2", "Wirkleistung  (Phase 2)"},
	{Obis(1, 0, 61, 7, DC, DC), "power-l3", "Wirkleistung  (Phase 3)"},

	{Obis(1, 0, 12, 7, DC, DC), "voltage", "Spannung      (Mittelwert)"},
	{Obis(1, 0, 32, 7, DC, DC), "voltage-l1", "Spannung      (Phase 1)"},
	{Obis(1, 0, 52, 7, DC, DC), "voltage-l2", "Spannung      (Phase 2)"},
	{Obis(1, 0, 72, 7, DC, DC), "voltage-l3", "Spannung      (Phase 3)"},

	{Obis(1, 0, 11, 7, DC, DC), "current", "Stromstaerke  (Summe)"},
	{Obis(1, 0, 31, 7, DC, DC), "current-l1", "Stromstaerke  (Phase 1)"},
	{Obis(1, 0, 51, 7, DC, DC), "current-l2", "Stromstaerke  (Phase 2)"},
	{Obis(1, 0, 71, 7, DC, DC), "current-l3", "Stromstaerke  (Phase 3)"},

	{Obis(1, 0, 14, 7, 0, DC), "frequency", "Netzfrequenz"},
	{Obis(1, 0, 12, 7, 0, DC), "powerfactor", "Leistungsfaktor"},

	{Obis(0, 0, 96, 1, DC, DC), "device", "Zaehler Seriennr."},
	{Obis(1, 0, 96, 5, 5, DC), "status", "Zaehler Status"},

	{Obis(1, 0, 1, 8, DC, DC), "counter", "Zaehlerstand Wirkleistung"},
	{Obis(1, 0, 2, 8, DC, DC), "counter-out", "Zaehlerstand Lieferg."},

	//	Easymeter
	//	ESYQ3B (Easymeter Q3B)
	{Obis(1, 0, 1, 8, 1, DC), "esy-counter-t1", "Active Power Counter Tariff 1"},
	{Obis(1, 0, 1, 8, 2, DC), "esy-counter-t2", "Active Power Counter Tariff 2"},
	//	{Obis(129, 129, 199, 130,   3,  DC), "",		""}, // ???
	//	ESYQ3D (Easymeter Q3D)
	//	{Obis(  0,   0,   0,   0,   0,  DC), "",		""}, // ???

	//	HAG eHZ010C_EHZ1WA02 (Hager eHz)
	{Obis(1, 0, 0, 0, 0, DC), "hag-id", "Eigentumsnr."},
	{Obis(1, 0, 96, 50, 0, 0), "hag-status",
	 "Netz Status"}, // bitcodiert: Drehfeld, Anlaufschwelle, Energierichtung
	{Obis(1, 0, 96, 50, 0, 1), "hag-frequency", "Netz Periode"}, // hexadezimal (Einheit 1/100 ms)
	{Obis(1, 0, 96, 50, 0, 2), "hag-temp", "aktuelle Chiptemperatur"}, // hexadezimal, Einheit °C
	{Obis(1, 0, 96, 50, 0, 3), "hag-temp-min", "minimale Chiptemperatur"},
	{Obis(1, 0, 96, 50, 0, 4), "hag-temp-avg", "gemittelte Chiptemperatur"},
	{Obis(1, 0, 96, 50, 0, 5), "hag-temp-max", "maximale Chiptemperatur"},
	{Obis(1, 0, 96, 50, 0, 6), "hag-check", "Kontrollnr."},
	{Obis(1, 0, 96, 50, 0, 7), "hag-diag", "Diagnose"},

	//	Schweizer VNBs lt https://github.com/volkszaehler/vzlogger/issues/205
	{Obis(255, 255, 16, 8, 0, 255), "lg-counter-et", "Sum active energy (Total)"},
	{Obis(255, 255, 16, 8, 1, 255), "lg-counter-ht", "Sum active energy (T1)"},
	{Obis(255, 255, 16, 8, 2, 255), "lg-counter-lt", "Sum active energy (T2)"},

	//	Stop condition for iterator
	{Obis(DC, DC, DC, DC, DC, DC), NULL, NULL},
};

obis_alias_t *obis_get_aliases() { return aliases; }

Obis::Obis(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e,
		   unsigned char f) {
	_obisId._raw[0] = a;
	_obisId._raw[1] = b;
	_obisId._raw[2] = c;
	_obisId._raw[3] = d;
	_obisId._raw[4] = e;
	_obisId._raw[5] = f;
}

Obis::Obis(const char *strClear) {
	if (parse(strClear) != SUCCESS) {
		// check alias
		if (lookup_alias(strClear) == SUCCESS) {
		} else {
			throw vz::VZException("Parse ObisString failed.");
		}
	}
}

Obis::Obis() {
	// to be more consistent we initialize with DC/255.
	_obisId._raw[0] = DC;
	_obisId._raw[1] = DC;
	_obisId._raw[2] = DC;
	_obisId._raw[3] = DC;
	_obisId._raw[4] = DC;
	_obisId._raw[5] = DC;
}

int Obis::parse(const char *str) {
	enum { A = 0, B, C, D, E, F };

	char byte; // currently processed byte
	int num;
	int field;
	int len = strlen(str);
	int digit = 0;
	bool has_sc = false;

	num = byte = 0;
	field = -1;
	memset(&_obisId._raw, DC, 6); // initialize as "not given"

	// format: "A-B:C.D.E[*&]F"
	// fields A, B, E, F are optional
	// fields C & D are mandatory
	for (int i = 0; i < len; i++) {
		byte = str[i];
		digit++; // count number of digits for this field

		if (isdigit(byte)) {
			if (has_sc)
				return ERR;                  // no F1,... allowed.
			num = (num * 10) + (byte - '0'); // parse digits
		} else if (byte == 'C') {
			num = SC_C;
			has_sc = true;
			if (digit > 1)
				return ERR; // no CC, FF,... allowed, not even " C" (space and special char)
		} else if (byte == 'F') {
			num = SC_F;
			has_sc = true;
			if (digit > 1)
				return ERR;
		} else if (byte == 'L') {
			num = SC_L;
			has_sc = true;
			if (digit > 1)
				return ERR;
		} else if (byte == 'P') {
			has_sc = true;
			num = SC_P;
			if (digit > 1)
				return ERR;
		} else {
			if (byte == '-' && field < A) { // end of field A
				field = A;
			} else if (byte == ':' && field < B) { // end of field B
				field = B;
			} else if (byte == '.' && field < D) { // end of field C & D
				field = (field < C) ? C : D;
			} else if ((byte == '*' || byte == '&') &&
					   field == D) { // end of field E, start of field F
				field = E;
			} else {
				return ERR;
			}

			if ((num < 0) || (num > 255))
				return ERR; // sanity check
			_obisId._raw[field] = num;
			num = 0;
			digit = 0;
			has_sc = false;
		}
	}

	// set last field
	_obisId._raw[++field] = num;

	// fields C & D are mandatory
	return (field < D) ? ERR : SUCCESS;
}

int Obis::lookup_alias(const char *alias) {
	for (const obis_alias_t *it = aliases; it != NULL && !it->id.isAllNotGiven(); it++) {
		if (strcmp(it->name, alias) == 0) {
			*this = it->id;
			return SUCCESS;
		}
	}
	return ERR_NOT_FOUND;
}

const std::string Obis::toString() {
	std::ostringstream oss;
	oss << (int)_obisId.groups.media << "-" << (int)_obisId.groups.channel << ":"
		<< (int)_obisId.groups.indicator << "." << (int)_obisId.groups.mode << "."
		<< (int)_obisId.groups.quantities << "*" << (int)_obisId.groups.storage;
	return oss.str();
}

size_t Obis::unparse(char *buffer, size_t n) {
	return snprintf(buffer, n, "%i-%i:%i.%i.%i*%i", _obisId.groups.media, _obisId.groups.channel,
					_obisId.groups.indicator, _obisId.groups.mode, _obisId.groups.quantities,
					_obisId.groups.storage);
}

bool Obis::operator==(const Obis &rhs) const {
	for (int i = 0; i < 6; i++) {
		if (_obisId._raw[i] == rhs._obisId._raw[i]) { // DC/255/0xff not treated as wildcard anymore
			continue;                                 // skip equal
		} else if (_obisId._raw[i] < rhs._obisId._raw[i]) {
			return 0;
		} else if (_obisId._raw[i] > rhs._obisId._raw[i]) {
			return 0;
		}
	}

	return 1; // equal
}

bool Obis::isAllNotGiven() const {
	return *this == Obis(); // compare this one with empty one from default constructor
}

bool Obis::isManufacturerSpecific() const {
	return ((_obisId.groups.channel >= 128 && _obisId.groups.channel <= 199) ||
			(_obisId.groups.indicator >= 128 && _obisId.groups.indicator <= 199) ||
			(_obisId.groups.indicator == 240) ||
			(_obisId.groups.mode >= 128 && _obisId.groups.mode <= 254) ||
			(_obisId.groups.quantities >= 128 && _obisId.groups.quantities <= 254) ||
			(_obisId.groups.storage >= 128 && _obisId.groups.storage <= 254));
}

bool Obis::isValid() const {
	// check validity according to V2.2 from 1.4.2013:
	// This is just a basic sanity check as the OBIS are not strictly defined.

	// A: 0..9
	switch (_obisId._raw[0]) {
	case 0: // abstract objects
	case 1: // electr.
	case 2: // not def?
	case 3: // not def?
	case 4: // Heizkostenvert.
	case 5: // Kaelte
	case 6: // Waerme
	case 7: // Gas
	case 8: // water (cold)
	case 9: // water (warm)
		break;
	default:
		return false;
	}

	// B 0 - 64
	if (_obisId._raw[1] > 64)
		return false;

	// don't check C, D, E yet, has many special values
	// F 0..99 or ff
	if ((_obisId._raw[5] != 0xff) && (_obisId._raw[5] > 99))
		return false;
	return true;
}
