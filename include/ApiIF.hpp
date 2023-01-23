/***********************************************************************/
/** @file ApiIF.hpp
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

#ifndef _ApiIF_hpp_
#define _ApiIF_hpp_

#include <string>

#include <Channel.hpp>
#include <common.h>

namespace vz {
class ApiIF {
  public:
	typedef vz::shared_ptr<ApiIF> Ptr;

	ApiIF(Channel::Ptr ch) : _ch(ch) {}
	virtual ~ApiIF(){};

	/**
	 * @brief send measurement values to middleware
	 * to be implemented specific API.
	 **/
	virtual void send() = 0;
	virtual void register_device() = 0;

  protected:
	Channel::Ptr channel() { return _ch; }

  private:
	Channel::Ptr _ch; /**< pointer to channel where API belongs to */
};                    // class ApiIF

} // namespace vz
#endif /* _ApiIF_hpp_ */
