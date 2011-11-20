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

#define OBIS_STR_LEN (6*3+5+1)

/* regex: A-BB:CC.DD.EE([*&]FF)? */
typedef union {
	unsigned char raw[6];
	struct {
		unsigned char media, channel, indicator, mode, quantities;
		unsigned char storage;	/* not used in Germany */
	} groups;
} obis_id_t;

typedef struct {
	obis_id_t id;
	char *name;
	char *desc;
} obis_alias_t;

/* prototypes */
obis_id_t * obis_init(obis_id_t *id, unsigned char *raw);

const obis_alias_t * obis_get_aliases();
int obis_parse(const char *str, obis_id_t *id);
int obis_lookup_alias(const char *alias, obis_id_t *id);
int obis_unparse(obis_id_t id, char *buffer, size_t n);
int obis_compare(obis_id_t a, obis_id_t b);

int obis_is_manufacturer_specific(obis_id_t id);
int obis_is_null(obis_id_t id);

#endif /* _OBIS_H_ */
