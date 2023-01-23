/**
 * Header file for volkszaehler.org API calls
 *
 * @author Kai Krueger <kai.krueger@itwm.fraunhofer.de>
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
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

#include <iostream>
#include <string.h>

#include <api/CurlResponse.hpp>
#include <common.h>

void vz::api::CurlResponse::header_callback(const std::string &data) {}

void vz::api::CurlResponse::write_callback(const std::string &data) { _response_data.append(data); }

void vz::api::CurlResponse::debug_callback(curl_infotype type, const std::string &data_str) {
	const char *data = data_str.c_str();

	char *end = strchr((char *)data, '\n');

	if (data == end)
		return; /* skip empty line */

	switch (type) {
	case CURLINFO_TEXT:
	case CURLINFO_END:
		if (end)
			*end = '\0'; /* terminate without \n */
		print((log_level_t)(log_debug + 5), "CURL: %.*s", "CURL", (int)data_str.size(), data);
		break;

	case CURLINFO_SSL_DATA_IN:
		print((log_level_t)(log_debug + 5), "CURL: Received %lu bytes", "CURL",
			  (unsigned long)data_str.size());
		break;
	case CURLINFO_DATA_IN:
		print((log_level_t)(log_debug + 5), "CURL: Received %lu bytes", "CURL",
			  (unsigned long)data_str.size());
		print((log_level_t)(log_debug + 5), "CURL: Received '%s' bytes", "CURL", data);
		break;

	case CURLINFO_SSL_DATA_OUT:
		print((log_level_t)(log_debug + 5), "CURL: Sent %lu bytes.. ", "CURL",
			  (unsigned long)data_str.size());
		break;
	case CURLINFO_DATA_OUT:
		print((log_level_t)(log_debug + 5), "CURL: Sent %lu bytes.. ", "CURL",
			  (unsigned long)data_str.size());
		print((log_level_t)(log_debug + 5), "CURL: Sent '%s' bytes", "CURL", data);
		break;

	case CURLINFO_HEADER_IN:
	case CURLINFO_HEADER_OUT:
		print((log_level_t)(log_debug + 5), "CURL: Header '%s' bytes", "CURL", data);
		break;
	}
}

int vz::api::CurlResponse::progress_callback(void *clientp, double dltotal, double dlnow,
											 double ultotal, double ulnow) {
	return 0;
}

void vz::api::CurlResponse::split_response(size_t n) {
	_header = _response_data.substr(0, n);
	_body = _response_data.substr(n);
}
