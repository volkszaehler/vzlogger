/**
 * Reading related functions
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

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "reading.h"
#include "meter.h"

char * reading_id_registry(const char *str) {
	static list_t strings;

	char *found = NULL;

	/* linear search in string list */
	foreach(strings, it, char *) {
		if (strcmp(it, str) == 0) {
			found = it;
			break;
			}
	}

	if (!found) {
		found = strdup(str);
		list_push(strings, found);
	}

	return found;
}

int reading_id_compare(meter_protocol_t protocol, reading_id_t a, reading_id_t b) {
	switch (protocol) {
		case meter_protocol_d0:
		case meter_protocol_sml:
			return obis_compare(a.obis, b.obis);

		case meter_protocol_fluksov2:
			return !(a.channel == b.channel);

		case meter_protocol_file:
		case meter_protocol_exec:
			/* we only need to compare the base pointers here,
			   because each identifer exists only once in the registry
			   and has therefore a unique pointer */
			return !(a.string == b.string);

		default:
			/* no channel id, adding all readings to buffer */
			return 0; /* equal */
	}
}

int reading_id_parse(meter_protocol_t protocol, reading_id_t *id, const char *string) {
	switch (protocol) {
		case meter_protocol_d0:
		case meter_protocol_sml:
			if (obis_parse(string, &id->obis) != SUCCESS) {
				if (obis_lookup_alias(string, &id->obis) != SUCCESS) {
					return ERR;
				}
			}
			break;

		case meter_protocol_fluksov2: {
			char type[13];
			int channel;

			int ret = sscanf(string, "sensor%u/%12s", &channel, type);
			if (ret != 2) {
				return ERR;
			}

			id->channel = channel + 1; /* increment by 1 to distinguish between +0 and -0 */

			if (strcmp(type, "consumption") == 0) {
				id->channel *= -1;
			}
			else if (strcmp(type, "power") != 0) {
				return ERR;
			}
			break;
		}

		case meter_protocol_file:
		case meter_protocol_exec:
			id->string = reading_id_registry(string);
			break;

		default: /* ignore other protocols which do not provide id's */
			break;
	}

	return SUCCESS;
}

size_t reading_id_unparse(meter_protocol_t protocol, reading_id_t id, char *buffer, size_t n) {
	switch (protocol) {
		case meter_protocol_d0:
		case meter_protocol_sml:
			obis_unparse(id.obis, buffer, n);
			break;

		case meter_protocol_fluksov2:
			snprintf(buffer, n, "sensor%u/%s", abs(id.channel) - 1, (id.channel > 0) ? "power" : "consumption");
			break;

		case meter_protocol_file:
		case meter_protocol_exec:
			if (id.string != NULL) {
				strncpy(buffer, id.string, n);
				break;
			}

		default:
			buffer[0] = '\0';
	}

	return strlen(buffer);
}

double tvtod(struct timeval tv) {
	return tv.tv_sec + tv.tv_usec / 1e6;
}

struct timeval dtotv(double ts) {
	double integral;
	double fraction = modf(ts, &integral);

	struct timeval tv = {
		.tv_usec = (long int) (fraction * 1e6),
		.tv_sec = (long int) integral
	};

	return tv;
}
