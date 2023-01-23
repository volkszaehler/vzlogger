/***********************************************************************/
/** @file MySmartGrid.hpp
 * Header file for volkszaehler.org API calls
 *
 *  @author Kai Krueger
 *  @date   2012-03-15
 *  @email  kai.krueger@itwm.fraunhofer.de
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
 * @package vzlogger
 * @license http://opensource.org/licenses/gpl-license.php GNU Public License
 *
 * (C) Fraunhofer ITWM
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

#ifndef _MySmartGrid_hpp_
#define _MySmartGrid_hpp_

#include <ApiIF.hpp>
#include <Options.hpp>
#include <Reading.hpp>
#include <api/CurlIF.hpp>
#include <api/CurlResponse.hpp>

namespace vz {
namespace api {
const short chn_type_device = 10;
const short chn_type_sensor = 20;

class MySmartGrid : public ApiIF {
  public:
	typedef vz::shared_ptr<MySmartGrid> Ptr;

	MySmartGrid(Channel::Ptr ch, std::list<Option> options);
	~MySmartGrid();

	void send();

	void register_device();

	const std::string middleware() const { return _middleware; }

  private:
	void _send(const std::string &url, json_object *json_obj);

	/**
	 * Parses JSON encoded exception and stores describtion in err
	 */
	void api_parse_exception(char *err, size_t n);

	/**
	 *  api configured as device
	 */
	json_object *_apiDevice(Buffer::Ptr buf);

	/**
	 *  api configured as sensor
	 */
	json_object *_apiSensor(Buffer::Ptr buf);

	json_object *_json_object_registration();
	json_object *_json_object_heartbeat();
	json_object *_json_object_event(Buffer::Ptr buf);
	json_object *_json_object_sensor(const std::string &sensorName);
	json_object *_json_object_measurements(Buffer::Ptr buf);

	void _api_header();

	CurlResponse *response() { return _response.get(); }

	void convertUuid(const std::string uuidIn, std::string &uuidOut);
	void convertUuid(const std::string uuid);
	const char *uuid() const { return _uuid.c_str(); }
	const char *secretKey() const { return _secretKey.c_str(); }

	int interval() const { return _interval; }
	time_t first_ts() const { return _first_ts; }

  private:
	std::string _middleware; /**< url to MySmartGrid Server */
	std::string _uuid;       /**< unique sensor id */
	std::string _deviceId;   /**< deviceid */
	std::string _secretKey;  /**< secretkey for signing messages */
	int _interval;           /**<  time between 2 logmessages (sec.) */
	short _channelType;      /**< Type of channel device or sensor */
	unsigned int _scaler;    /**< scaling faktor for values */

	CurlIF _curlIF;
	CurlResponse::Ptr _response;

	// Volatil
	std::list<Reading> _values;

	time_t _first_ts;
	long _first_counter;
	long _last_counter;

}; // class MySmartGrid

} // namespace api
} // namespace vz
#endif /* _MySmartGrid_hpp_ */
