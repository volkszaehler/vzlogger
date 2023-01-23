/**
 * Meter interface
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

#ifndef _meter_protocol_hpp_
#define _meter_protocol_hpp_

typedef enum meter_procotol {
	meter_protocol_none = 0,
	meter_protocol_file = 1,
	meter_protocol_exec,
	meter_protocol_random,
	meter_protocol_s0,
	meter_protocol_d0,
	meter_protocol_sml,
	meter_protocol_fluksov2,
	meter_protocol_ocr,
	meter_protocol_w1therm,
	meter_protocol_oms,
} meter_protocol_t;
#endif /* _meter_protocol_hpp_ */
