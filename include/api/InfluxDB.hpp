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

#include <curl/curl.h>
#include <common.h>
#include <ApiIF.hpp>
#include <api/CurlIF.hpp>
#include <api/CurlResponse.hpp>
#include <Options.hpp>

namespace vz {
	namespace api {



		class InfluxDB : public ApiIF {
		public:
			typedef vz::shared_ptr<ApiIF> Ptr;

			InfluxDB(Channel::Ptr ch, std::list<Option> options);
			~InfluxDB();

			void send();

			void register_device();

		private:
			std::string _host;
			std::string _username;
			std::string _password;
			std::string _database;
			std::string _measurement_name;
			std::string _url;
			unsigned int _curl_timeout;
			std::list<Reading> _values;
			CurlResponse *response()   { return _response.get(); }
			CurlResponse::Ptr _response;

			typedef struct {
				CURL *curl;
				struct curl_slist *headers;
			} api_handle_t;
			api_handle_t _api;

			// typedef struct {
			// 	char *data;
			// 	size_t size;
			// } CURLresponse;

		// static int curl_custom_debug_callback(CURL *curl, curl_infotype type, char *data, size_t size, void *custom);
		// static size_t curl_custom_write_callback(void *ptr, size_t size, size_t nmemb, void *data);
		}; // class InfluxDB

	} // namespace api
} // namespace vz
#endif // _InfluxDB_hpp_
