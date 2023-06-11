/**
 * Header file for volkszaehler.org API calls
 *
 * @author Steffen Vogel <info@steffenvogel.de>
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

#ifndef _API_H_
#define _API_H_

#include <curl/curl.h>
#include <json-c/json.h>
#include <stddef.h>
#include <sys/time.h>

#include "buffer.h"
#include "channel.h"

typedef struct {
	char *data;
	size_t size;
} CURLresponse;

typedef struct {
	CURL *curl;
	struct curl_slist *headers;
} api_handle_t;

namespace vz {

class Api {
  public:
	Api(Channel::Ptr ch);
	~Api();
	CURL *curl() { return _api.curl; }

  private:
	Channel::Ptr _ch;
	api_handle_t _api;
};
} // namespace vz

/**
 * Reformat CURLs debugging output
 */
int curl_custom_debug_callback(CURL *curl, curl_infotype type, char *data, size_t size,
							   void *custom);

size_t curl_custom_write_callback(void *ptr, size_t size, size_t nmemb, void *data);

/**
 * Create JSON object of tuples
 *
 * @param buf	the buffer our readings are stored in (required for mutex)
 * @param first	the first tuple of our linked list which should be encoded
 * @param last	the last tuple of our linked list which should be encoded
 * @return the json_object (has to be free'd)
 */
json_object *api_json_tuples(Buffer::Ptr buf);
// Reading *first, Reading *last);

/**
 * Parses JSON encoded exception and stores describtion in err
 */
void api_parse_exception(CURLresponse response, char *err, size_t n);

#endif /* _API_H_ */
