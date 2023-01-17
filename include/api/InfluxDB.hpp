/***********************************************************************/
/** @file InfluxDB.hpp
 * Header file for InfluxDB API calls
 *
 * @author Stefan Kuntz
 * @email  Stefan.github@gmail.com
 * @copyright Copyright (c) 2017, The volkszaehler.org project
 * @package vzlogger
 * @license http://opensource.org/licenses/gpl-license.php GNU Public License
 **/
/*---------------------------------------------------------------------*/

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

#ifndef _InfluxDB_hpp_
#define _InfluxDB_hpp_

#include <ApiIF.hpp>
#include <Options.hpp>
#include <api/CurlIF.hpp>
#include <api/CurlResponse.hpp>
#include <common.h>
#include <curl/curl.h>

namespace vz {
namespace api {
class InfluxDB : public ApiIF {
  public:
	typedef vz::shared_ptr<ApiIF> Ptr;

	InfluxDB(const Channel::Ptr &ch, const std::list<Option> &options);
	~InfluxDB();

	void send();

	void register_device();

  private:
	CurlResponse *response() { return _response.get(); }

  private:
	std::string _host;
	std::string _username;
	std::string _token;
	struct curl_slist *_token_header;
	std::string _organization;
	std::string _password;
	std::string _database;
	std::string _measurement_name;
	std::string _tags;
	std::string _url;
	int _max_batch_inserts;
	int _max_buffer_size;
	unsigned int _curl_timeout;
	bool _send_uuid;
	bool _ssl_verifypeer;
	std::list<Reading> _values;
	CurlResponse::Ptr _response;

	typedef struct {
		CURL *curl;
		struct curl_slist *headers;
	} api_handle_t;
	api_handle_t _api;
}; // class InfluxDB

} // namespace api
} // namespace vz
#endif // _InfluxDB_hpp_
