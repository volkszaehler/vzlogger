/**
 * Reading related functions
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

#ifndef _READING_H_
#define _READING_H_

#include <sstream>
#include <string>

#include <string.h>
#include <sys/time.h>

#include "Obis.hpp"
#include <meter_protocol.hpp>
#include <shared_ptr.hpp>

#define MAX_IDENTIFIER_LEN 255

/* Identifiers */
class ReadingIdentifier {
  public:
	typedef vz::shared_ptr<ReadingIdentifier> Ptr;
	virtual ~ReadingIdentifier(){};

	virtual size_t unparse(char *buffer, size_t n) = 0;
	bool operator==(ReadingIdentifier const &other) const { return other.isEqual(this); }
	virtual const std::string toString() = 0;

  protected:
	explicit ReadingIdentifier(){};
	virtual bool isEqual(ReadingIdentifier const *other) const = 0;

  private:
	// ReadingIdentifier (const ReadingIdentifier& original);
	// ReadingIdentifier& operator= (const ReadingIdentifier& rhs);
};

namespace detail {
template <class T> bool isEqual(const T &obj, const ReadingIdentifier *other) {
	typedef typename std::add_pointer<typename std::add_const<T>::type>::type Pointer;
	Pointer ptr(dynamic_cast<Pointer>(other));
	return (ptr != nullptr) && (obj == *ptr);
}
} // namespace detail

class ObisIdentifier : public ReadingIdentifier {

  public:
	typedef vz::shared_ptr<ObisIdentifier> Ptr;

	ObisIdentifier() {}
	ObisIdentifier(Obis obis) : _obis(obis) {}
	virtual ~ObisIdentifier(){};

	size_t unparse(char *buffer, size_t n);
	bool operator==(ObisIdentifier const &other) const { return _obis == other.obis(); }

	const std::string toString() {
		std::ostringstream oss;
		oss << "ObisIdentifier:" << _obis.toString();
		return oss.str();
	};

	const Obis &obis() const { return _obis; }

  private:
	// ObisIdentifier (const ObisIdentifier& original);
	// ObisIdentifier& operator= (const ObisIdentifier& rhs);

  protected:
	virtual bool isEqual(const ReadingIdentifier *other) const {
		return detail::isEqual(*this, other);
	}

	Obis _obis;
};

class StringIdentifier : public ReadingIdentifier {
  public:
	StringIdentifier() {}
	StringIdentifier(std::string s) : _string(s) {}

	void parse(const char *buffer);
	size_t unparse(char *buffer, size_t n);
	bool operator==(StringIdentifier const &other) const { return _string == other._string; }
	const std::string toString() {
		std::ostringstream oss;
		oss << "StringIdentifier:";
		return oss.str();
	};

  protected:
	virtual bool isEqual(const ReadingIdentifier *other) const {
		return detail::isEqual(*this, other);
	}

	std::string _string;
};

class ChannelIdentifier : public ReadingIdentifier {

  public:
	ChannelIdentifier() {}
	ChannelIdentifier(int channel) : _channel(channel) {}

	void parse(const char *string);
	size_t unparse(char *buffer, size_t n);
	bool operator==(ChannelIdentifier const &other) const { return _channel == other._channel; }

	const std::string toString() {
		std::ostringstream oss;
		oss << "ChannelIdentifier:";
		return oss.str();
	};

  protected:
	virtual bool isEqual(const ReadingIdentifier *other) const {
		return detail::isEqual(*this, other);
	}

	int _channel;
};

class NilIdentifier : public ReadingIdentifier {
  public:
	NilIdentifier() {}
	size_t unparse(char *buffer, size_t n);
	bool operator==(NilIdentifier const &) const { return true; }
	const std::string toString() {
		std::ostringstream oss;
		oss << "NilIdentifier";
		return oss.str();
	};

  protected:
	virtual bool isEqual(const ReadingIdentifier *other) const {
		return detail::isEqual(*this, other);
	}
};

class Reading {

  public:
	typedef vz::shared_ptr<Reading> Ptr;
	Reading();
	Reading(ReadingIdentifier::Ptr pIndentifier);
	Reading(double pValue, struct timeval pTime, ReadingIdentifier::Ptr pIndentifier);
	Reading(const Reading &orig);
	Reading &operator=(const Reading &orig);

	bool deleted() const { return _deleted; }
	void mark_delete() { _deleted = true; }
	void reset() { _deleted = false; }

	void value(const double &v) { _value = v; }
	double value() const { return _value; }

	int64_t time_ms() const { return ((int64_t)_time.tv_sec) * 1e3 + (_time.tv_usec / 1e3); };
	long time_s() const { return _time.tv_sec; }; // return only the seconds (always rounding down)
	void time() { gettimeofday(&_time, NULL); }
	void time(struct timeval const &v) { _time = v; }
	void time(struct timespec const &v) {
		_time.tv_sec = v.tv_sec;
		_time.tv_usec = v.tv_nsec / 1e3;
	}
	// not needed yet: void time_from_ms( int64_t &ms );
	void time_from_double(double const &d);

	void identifier(ReadingIdentifier *rid) { _identifier.reset(rid); }
	const ReadingIdentifier::Ptr identifier() { return _identifier; }

	/**
	 * Print identifier to buffer for debugging/dump
	 *
	 * @return the amount of bytes used in buffer
	 */
	size_t unparse(/*meter_protocol_t protocol,*/ char *buffer, size_t n);

	bool operator==(const Reading &rhs) const {
		return (_deleted == rhs._deleted) && (_value == rhs._value) &&
			   (_time.tv_sec == rhs._time.tv_sec) && (_time.tv_usec == rhs._time.tv_usec);
	}

  protected:
	bool _deleted;
	double _value;
	struct timeval _time;
	ReadingIdentifier::Ptr _identifier;
};

/**
 * Parse identifier by a given string and protocol
 *
 * @param protocol the given protocol context in which the string should be parsed
 * @param string the string-encoded identifier
 * @return 0 on success, < 0 on error
 */
ReadingIdentifier::Ptr reading_id_parse(meter_protocol_t protocol, const char *string);

#endif /* _READING_H_ */
