/**
 * Main header file
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

#ifndef _VZLOGGER_H_
#define _VZLOGGER_H_

#include <pthread.h>
#include <vector>

#include "Config_Options.hpp"
#include "Meter.hpp"
#include "Channel.hpp"

using namespace std;

/* prototypes */
void quit(int sig);
void daemonize();

void show_usage(char ** argv);
void show_aliases();

int options_parse(int argc, char *argv[], Config_Options *options);

void register_device();

#endif /* _VZLOGGER_H_ */
