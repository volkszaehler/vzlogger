#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <sstream>
#include <iostream>
#include <list>
#include <json/json.h>

class Option {

public:
	/* subset of json_type's */
	typedef enum {
		type_boolean = 1,
		type_double,
		type_int,
		type_string = 6
	} type_t;

	Option(const char *key, struct json_object *jso);
	Option(const char *key, char *value);
	Option(const char *key, int value);
	Option(const char *key, double value);
	Option(const char *key, bool value);

	virtual ~Option();

	const std::string key() const { return _key; }
	operator const char *() const;
	operator int() const;
	operator double() const;
	operator bool() const;

	const type_t type() const { return _type; }

	const std::string toString() const {
		std::ostringstream oss;
		oss << "Option <"<< key() << ">=<type="<< type()<< ">=<val=";
		switch(type()) {
				case type_boolean: oss<< (bool)(*this);break;
				case type_double:  oss<< (double)(*this); break;
				case type_int:     oss<< (int)(*this); break;
				case type_string:  oss<< (const char*)(*this); break;
		}
		oss << ">";
		return oss.str();
	}


private:
	Option(const char *key);

	std::string _key;
	type_t _type;

	std::string _value_string;

	union {
		const char *string;
		int integer;
		double floating;
		int boolean:1;
	} value;

};

inline
std::ostream & operator << (std::ostream &s, const Option &l) {
	s<< l.toString();
	return s;
}


class OptionList { //: public List<Option> {

public:
	typedef std::list<Option>::iterator iterator;
	typedef std::list<Option>::const_iterator const_iterator;

	const Option& lookup(std::list<Option> options, const std::string &key);
	const char  *lookup_string(std::list<Option> options, const char *key);
	const int    lookup_int(std::list<Option> options, const char *key);
	const bool   lookup_bool(std::list<Option> options, const char *key);
	const double lookup_double(std::list<Option> options, const char *key);

	void dump(std::list<Option> options);

	void parse();

protected:

};

#endif /* _OPTIONS_H_ */
