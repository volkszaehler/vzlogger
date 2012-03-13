/**
 * Protocol interface
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011, The volkszaehler.org project
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

#include <vector>
#include <list>

#include <common.h>
#include <shared_ptr.hpp>
#include <reading.h>
#include <options.h>

namespace vz {
  namespace protocol {
  class Protocol {
  public:
    typedef vz::shared_ptr<Protocol> Ptr;

    Protocol(const std::string &name, std::list<Option> options) : _name(name) {};
    Protocol(std::list<Option> options) : _name("") {};
    Protocol(const Protocol &proto) {  std::cout<<"====>Protocol - copy!" << std::endl; }
    Protocol& operator=(const Protocol&proto) {  std::cout<<"====>Protocol - equal!" << std::endl; return (*this); }

    
    virtual ~Protocol() {  std::cout<<"Protocol::~Protocol()\n";};

    //virtual void   init(std::list<Option> options) = 0;
    virtual int    open() = 0;
    virtual int    close() = 0;
    virtual size_t read(std::vector<Reading> &rds, size_t n) = 0;

    const std::string &name() { return _name; }
    
  private:
    std::string _name;
    
  };
  
    
} // namespace protocol
} // namespace vz

#endif /* _protocol_hpp_ */
