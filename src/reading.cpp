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
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "exception.h"
#include "reading.h"

/* StringIdentifier */
bool StringIdentifier::operator==(ReadingIdentifier &cmp) {
	return (strcmp(string, cmp.string) == 0);
}

void StringIdentifier::parse(const char *string) {
	string = strdup(string);
}

size_t StringIdentifier::unparse(char *buffer, size_t n) {
	strncpy(buffer, string, n);
	return strlen(buffer);
}

StringIdentifier::~StringIdentifier() {
	free(string);
}

/* ChannelIdentifier */
bool ChannelIdentifier::operator==(ReadingIdentifier &cmp) {
	return (channel == cmp.channel);
}

void ChannelIdentifier::parse(const char *string) {
	char type[13];
	int channel;
	int ret = sscanf(string, "sensor%u/%12s", &channel, type);

	if (ret != 2) {
		throw new Exception("Failed to parse channel identifier");
	}

	id->channel = channel + 1; /* increment by 1 to distinguish between +0 and -0 */

	if (strcmp(type, "consumption") == 0) {
		id->channel *= -1;
	}
	else if (strcmp(type, "power") != 0) {
		throw new Exception("Invalid channel type");
	}
}

size_t ChannelIdentifier::unparse(char *buffer, size_t n) {
	return snprintf(buffer, n, "sensor%u/%s", abs(id.channel) - 1, (id.channel > 0) ? "power" : "consumption");
}

/* Reading */
double Reading::tvtod(struct timeval tv) {
	return tv.tv_sec + tv.tv_usec / 1e6;
}

struct timeval Reading::dtotv(double ts) {
	double integral;
	double fraction = modf(ts, &integral);

	struct timeval tv;
	tv.tv_usec = (long int) (fraction * 1e6);
	tv.tv_sec = (long int) integral;

	return tv;
}
