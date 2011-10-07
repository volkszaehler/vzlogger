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

#include <string.h>

typedef enum {
	ABSTRACT	= 0,
	ELECTRIC	= 1,
	HEAT_COST	= 4,
	COOLING		= 5,
	HEATING		= 6,
	GAS		= 7,
	WATER_COLD	= 8,
	WATER_HOT	= 9
} obis_media_t;

typedef union {
	enum {
		GENERAL_PURPOSE		= 0,
		ACTIVE_POWER_IN		= 1,
		ACTIVE_POWER_OUT	= 2,
		REACTIVE_POWER_IN	= 3,
		REACTIVE_POWER_OUT	= 4,
		REACTIVE_POWER_Q1	= 5,
		REACTIVE_POWER_Q2	= 6,
		REACTIVE_POWER_Q3	= 7,
		REACTIVE_POWER_Q4	= 8,
		APPARENT_POWER_IN	= 9,
		APPARENT_POWER_OUT	= 10,
		CURRENT_ANY		= 11,
		VOLTAGE_ANY		= 12,
		POWER_FACTOR_OUT	= 13,
		SUPPLY_FREQUENCY	= 14,
		ACTIVE_POWER_Q1		= 17,
		ACTIVE_POWER_Q2		= 18,
		ACTIVE_POWER_Q3		= 19,
		ACTIVE_POWER_Q4		= 20,
		
		L1_ACTIVE_POWER_INT	= 21,
		L1_ACTIVE_POWER_OUT	= 22,
		
		ANGLES			= 81,
		UNITLESS		= 82,
		LOSS			= 83,
		
		L1_POWER_FACTOR_OUT	= 85,
		L2_POWER_FACTOR_OUT	= 86,
		L3_POWER_FACTOR_OUT	= 87,
		
		AMPERE_SQUARE_HOURS	= 88,
		VOLT_SQUARE_HOURS	= 89,
		
		SERVICE			= 96,
		ERROR			= 97,
		LIST			= 98,
		DATA_PROFILE		= 99
		
	} electric;
	
	int gas; /* as defined in DIN EN 13757-1 */
} obis_indicator_t;

/* regex: A-BB:CC.DD.EE(*FF)? */
typedef union {
	unsigned char raw[6];
	struct {
		unsigned char media;
		unsigned char channel;
		unsigned char indicator;
		unsigned char mode;
		unsigned char quantities;
		unsigned char storage;	/* not used in Germany */
	} groups;
} obis_id_t;

typedef struct {
	obis_id_t id;
	char *name;
	char *desc;
} obis_alias_t;

/* Prototypes */
obis_id_t obis_init(const unsigned char *raw);
obis_id_t obis_parse(const char *str);
obis_id_t obis_lookup_alias(const char *alias);
int obis_unparse(obis_id_t id, char *buffer);
int obis_compare(obis_id_t a, obis_id_t b);
int obis_is_manufacturer_specific(obis_id_t id);
int obis_is_null(obis_id_t id);

#endif /* _OBIS_H_ */
