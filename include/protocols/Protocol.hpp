/**
 * Protocol generic interface
 * To implement new meter protocol, derive from this class and implement
 * the specific code for open(), close() and read()
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
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

#ifndef _protocol_hpp_
#define _protocol_hpp_

#include <list>
#include <vector>

#include <Options.hpp>
#include <Reading.hpp>
#include <common.h>
#include <shared_ptr.hpp>

namespace vz {
namespace protocol {
class Protocol {
  public:
	typedef vz::shared_ptr<Protocol> Ptr;

	Protocol(const std::string &name) : _name(name){};

	virtual ~Protocol(){};

	virtual int open() = 0;
	virtual int close() = 0;
	virtual ssize_t read(std::vector<Reading> &rds, size_t n) = 0;
	virtual bool allowInterval() const {
		return true;
	} // default we allow interval (but S0 e.g disallows)

	const std::string &name() const { return _name; }

  private:
	std::string _name;

}; // class protocol
} // namespace protocol
} // namespace vz

#endif /* _protocol_hpp_ */
