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

#include <VZException.hpp>
#include <api/CurlIF.hpp>

vz::api::CurlIF::CurlIF() : _headers(0) {
	_curl = curl_easy_init();
	if (!_curl) {
		throw vz::VZException("CURL: cannot create handle.");
	}
}

vz::api::CurlIF::~CurlIF() {
	curl_easy_cleanup(_curl);
	if (_headers != NULL)
		curl_slist_free_all(_headers);
}

void vz::api::CurlIF::addHeader(const std::string value) {
	_headers = curl_slist_append(_headers, value.c_str());
}
void vz::api::CurlIF::clearHeader() {
	if (_headers != NULL)
		curl_slist_free_all(_headers);
	_headers = NULL;
}
void vz::api::CurlIF::commitHeader() {
	if (_headers != NULL)
		curl_easy_setopt(handle(), CURLOPT_HTTPHEADER, _headers);
}
