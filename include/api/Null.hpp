/***********************************************************************/
/** @file Null.hpp
 * Header file for null API calls
 *
 * @author Andreas Goetz
 * @date   2012-03-15
 * @email  cpuidle@gmx.de
 * @copyright Copyright (c) 2014 - 2023, The volkszaehler.org project
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

#ifndef _Null_hpp_
#define _Null_hpp_

#include <ApiIF.hpp>
#include <Options.hpp>

namespace vz {
namespace api {

class Null : public ApiIF {
  public:
	typedef vz::shared_ptr<ApiIF> Ptr;

	Null(Channel::Ptr ch, std::list<Option> options);
	~Null();

	void send();

	void register_device();

}; // class Null

} // namespace api
} // namespace vz
#endif // _Null_hpp_
