/*
 * @file VZException.hpp
 * @date   2011-08-17
 *
 * Created by Kai Krueger <kai.krueger@itwm.fraunhofer.de>
 *
 * Copyright (c) 2011 Fraunhofer ITWM
 * All rights reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _VZException_hpp_
#define _VZException_hpp_
#include <exception>
#include <string>

namespace vz {
class VZException : public std::exception {
  public:
	explicit VZException(const std::string &reason) : _reason(reason) {}
	virtual ~VZException() throw() {}
	virtual const char *what() const throw() { return reason().c_str(); }
	virtual const std::string &reason() const { return _reason; }

  protected:
	std::string _reason;
};

// Option invalid type
class InvalidTypeException : public vz::VZException {
  public:
	InvalidTypeException(const std::string &reason) : vz::VZException(reason) {}
	virtual ~InvalidTypeException() throw() {}

  protected:
};

// Option item not found
class OptionNotFoundException : public vz::VZException {
  public:
	OptionNotFoundException(const std::string &reason) : vz::VZException(reason) {}
	virtual ~OptionNotFoundException() throw() {}

  protected:
};

// Connection failed
class ConnectionException : public vz::VZException {
  public:
	ConnectionException(const std::string &reason) : vz::VZException(reason) {}
	virtual ~ConnectionException() throw() {}

  protected:
};

// Timedout
namespace connection {
class TimeOut : public ConnectionException {
	TimeOut(const std::string &reason) : ConnectionException(reason) {}
	virtual ~TimeOut() throw() {}
};

} // namespace connection

} // namespace vz
#endif /* _VZException_hpp_ */
