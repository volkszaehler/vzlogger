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

#ifndef _CurlResponse_hpp_
#define _CurlResponse_hpp_

#include <curl/curl.h>
#include <shared_ptr.hpp>

namespace vz {
namespace api {

class CurlResponse {
  public:
	typedef vz::shared_ptr<CurlResponse> Ptr;

	CurlResponse() : _header(""), _body("") { clear_response(); };
	~CurlResponse(){};

	const std::string get_response() { return _response_data; }
	const std::string body() const { return _body; }
	const std::string header() const { return _header; }

	void clear_response() { _response_data.clear(); }
	void split_response(size_t n);

	// callbacks
	void header_callback(const std::string &data);
	void write_callback(const std::string &data);
	void debug_callback(curl_infotype infotype, const std::string &data);
	int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal,
						  double ulnow);

  protected:
	void response(const std::string &v) { _response_data = v; }

  private:
	std::string _response_data;
	std::string _header;
	std::string _body;
};

} // namespace api
} // namespace vz
#endif /* _CurlResponse_hpp_ */
