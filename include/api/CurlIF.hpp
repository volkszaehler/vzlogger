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

#ifndef _CurlIF_hpp_
#define _CurlIF_hpp_

#include <string>

#include <curl/curl.h>

namespace vz {
namespace api {

class CurlIF {
  public:
	CurlIF();
	~CurlIF();

	CURL *handle() { return _curl; }

	void addHeader(const std::string value);
	void clearHeader();
	void commitHeader();

	CURLcode perform() { return curl_easy_perform(handle()); }

  private:
	CURL *_curl;
	struct curl_slist *_headers;
}; // class CurlIF

} // namespace api
} // namespace vz
#endif /* _CurlIF_hpp_ */
