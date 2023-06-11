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
#include <string>

#include <api/CurlCallback.hpp>
#include <api/CurlResponse.hpp>

size_t vz::api::CurlCallback::header_callback(char *ptr, size_t size, size_t nmemb,
											  void *userdata) {
	const std::string data(ptr, nmemb);

	CurlResponse *resp = static_cast<CurlResponse *>(userdata);
	resp->header_callback(data);

	return size * nmemb;
}

size_t vz::api::CurlCallback::write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
	const std::string data(ptr, nmemb);

	CurlResponse *resp = static_cast<CurlResponse *>(userdata);
	resp->write_callback(data);

	return size * nmemb;
}

size_t vz::api::CurlCallback::debug_callback(CURL *hCurl, curl_infotype infotype, char *info,
											 size_t size, void *userdata) {
	const std::string data(info, size);
	CurlResponse *resp = static_cast<CurlResponse *>(userdata);
	resp->debug_callback(infotype, data);

	return 0;
}

int vz::api::CurlCallback::progress_callback(void *clientp, double dltotal, double dlnow,
											 double ultotal, double ulnow) {
	return 0;
}
