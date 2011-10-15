/**
 * Parsing commandline options and channel list
 *
 * @author Steffen Vogel <info@steffenvogel.de>
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @package vzlogger
 * @license http://opensource.org/licenses/gpl-license.php GNU Public License
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

#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include <json/json.h>

#include "channel.h"
#include "list.h"
#include "vzlogger.h"

void parse_configuration(char *filename, list_t *assocs, options_t *opts);
channel_t * parse_channel(struct json_object *jso);
assoc_t * parse_meter(struct json_object *jso);

int check_type(char *key, struct json_object *jso, enum json_type type);

#endif /* _CONFIGURATION_H_ */
