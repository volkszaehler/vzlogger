/**
 * Reading related functions
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

#ifndef _READING_H_
#define _READING_H_

#include <string>
#include <sstream>

#include <sys/time.h>
#include <string.h>

#include "Obis.hpp"
#include <shared_ptr.hpp>
#include <meter_protocol.hpp>

#define MAX_IDENTIFIER_LEN 255

/* Identifiers */
class ReadingIdentifier {
public:
	typedef vz::shared_ptr<ReadingIdentifier> Ptr;
	virtual ~ReadingIdentifier(){};

	virtual size_t unparse(char *buffer, size_t n) = 0;
	virtual bool operator==( ReadingIdentifier &cmp);
	bool compare( ReadingIdentifier *lhs,  ReadingIdentifier *rhs);

	virtual const std::string toString()  = 0;

protected:
	explicit ReadingIdentifier() {};

private:
//ReadingIdentifier (const ReadingIdentifier& original);
//ReadingIdentifier& operator= (const ReadingIdentifier& rhs);
};

class ObisIdentifier : public ReadingIdentifier {

public:
	typedef vz::shared_ptr<ObisIdentifier> Ptr;

	ObisIdentifier() {}
	ObisIdentifier(Obis obis) : _obis(obis) {}
	virtual ~ObisIdentifier(){};

	size_t unparse(char *buffer, size_t n);
	bool operator==(ObisIdentifier &cmp);
	const std::string toString() {
		std::ostringstream oss;
		oss << "ObisItentifier:" << _obis.toString();
		return oss.str();
	};

	const Obis &obis() const { return _obis; }

private:
	//ObisIdentifier (const ObisIdentifier& original);
	//ObisIdentifier& operator= (const ObisIdentifier& rhs);

protected:
	Obis _obis;
};

class StringIdentifier : public ReadingIdentifier {
public:
	StringIdentifier() {}
	StringIdentifier(std::string s) : _string(s) {}

	void parse(const char *buffer);
	size_t unparse(char *buffer, size_t n);
	bool operator==(StringIdentifier &cmp);
	const std::string toString()  {
		std::ostringstream oss;
		oss << "StringItentifier:";
		return oss.str();
	};
protected:
	std::string _string;
};


class ChannelIdentifier : public ReadingIdentifier {

public:
	ChannelIdentifier() {}
	ChannelIdentifier(int channel) : _channel(channel) {}

	void parse(const char *string);
	size_t unparse(char *buffer, size_t n);
	bool operator==(ChannelIdentifier &cmp);
	const std::string toString()  {
		std::ostringstream oss;
		oss << "ChannelItentifier:";
		return oss.str();
	};

protected:
	int _channel;
};

class NilIdentifier : public ReadingIdentifier {
public:
	NilIdentifier() {}
	size_t unparse(char *buffer, size_t n);
	bool operator==(NilIdentifier &cmp);
	const std::string toString()  {
		std::ostringstream oss;
		oss << "NilIdentifier";
		return oss.str();
	};
private:
};

class Reading {

public:
	typedef vz::shared_ptr<Reading> Ptr;
	Reading();
	Reading(ReadingIdentifier::Ptr pIndentifier);
	Reading(double pValue, struct timeval pTime, ReadingIdentifier::Ptr pIndentifier);
	Reading(const Reading &orig);

	const bool deleted() const { return _deleted; }
	void  mark_delete()        { _deleted = true; }
	void  reset()              { _deleted = false; }

	void value(const double &v) { _value = v; }
	const double value() const  { return _value; }

	const  double tvtod() const;
	double tvtod(struct timeval tv);
	void time() { gettimeofday(&_time, NULL); }
	void time(struct timeval &v) { _time = v; }
	struct timeval dtotv(double ts);

	void identifier(ReadingIdentifier *rid)  { _identifier.reset(rid); }
	const ReadingIdentifier::Ptr identifier() { return _identifier; }

/**
 * Print identifier to buffer for debugging/dump
 *
 * @return the amount of bytes used in buffer
 */
    size_t unparse(/*meter_protocol_t protocol,*/ char *buffer, size_t n);

protected:
	bool   _deleted;
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
