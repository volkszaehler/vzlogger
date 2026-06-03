/***********************************************************************/
/** @file MQTT.hpp
 * Header file for the MQTT API.
 *
 * Publishes channel readings to an MQTT broker. The broker connection itself
 * (host, port, credentials, TLS, ...) is configured in the global "mqtt" config
 * block and managed by the global MqttClient (see mqtt.hpp). This class is the
 * per-channel api, selected via "api": "mqtt", so that mqtt benefits from
 * aggregation and the regular channel logging path (issue #550).
 *
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
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

#ifndef _MQTT_hpp_
#define _MQTT_hpp_

#ifdef ENABLE_MQTT

#include <ApiIF.hpp>
#include <Options.hpp>
#include <utility>
#include <vector>

namespace vz {
namespace api {

class MQTT : public ApiIF {
  public:
	typedef vz::shared_ptr<ApiIF> Ptr;

	MQTT(const Channel::Ptr &ch, const std::list<Option> &options);
	~MQTT();

	void send();

	void register_device();

  private:
	std::string _topicAgg; // topic for aggregated values
	std::string _topicRaw; // topic for raw values
	bool _sendAgg;         // channel uses aggregation -> publish to agg topic
	bool _sendRaw;         // publish to raw topic
	int _qos;
	bool _retain;
	bool _timestamp; // include a {timestamp,value} json payload instead of bare value

	bool _announced;             // uuid/id already announced
	std::string _announcePrefix; // topic prefix for announcements
	std::vector<std::pair<std::string, std::string>> _announceValues; // e.g. uuid, id

	int64_t _last_timestamp; // remember last sent timestamp (duplicates support)
	Reading *_lastReadingSent;

	std::string makePayload(int64_t timestamp, double value) const;
}; // class MQTT

} // namespace api
} // namespace vz

#endif // ENABLE_MQTT
#endif // _MQTT_hpp_
