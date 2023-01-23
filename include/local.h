/**
 * Header file for local interface
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

#ifndef _LOCAL_H_
#define _LOCAL_H_

#include <stdarg.h>     /* required for libMHD */
#include <stdint.h>     /* required for libMHD */
#include <sys/socket.h> /* required for libMHD */

#include <microhttpd.h>

#if MHD_VERSION >= 0x00097002
using MHD_RESULT = MHD_Result;
#else
using MHD_RESULT = int;
#endif

MHD_RESULT handle_request(void *cls, struct MHD_Connection *connection, const char *url,
						  const char *method, const char *version, const char *upload_data,
						  size_t *upload_data_size, void **con_cls);

class Channel;
void shrink_localbuffer(); // remove old data in the local buffer
void add_ch_to_localbuffer(Channel &ch);

#endif /* _LOCAL_H_ */
