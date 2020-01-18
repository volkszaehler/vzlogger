#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <iostream>
#include <json-c/json.h>
#include <list>
#include <sstream>

class Option {

  public:
	/* subset of json_type's */
	typedef enum {
		type_boolean = 1,
		type_double,
		type_int,
		type_object,
		type_array,
		type_string
	} type_t;

	Option(const char *key, struct json_object *jso);
	Option(const char *key, const char *value);
	Option(const char *pKey, const std::string &pValue);
	Option(const char *key, int value);
	Option(const char *key, double value);
	Option(const char *key, bool value);

	Option(const Option &); // copy constructor to refcount jso properly

	virtual ~Option();

	const std::string key() const { return _key; }
	operator const char *() const;
	operator int() const;
	operator double() const;
	operator bool() const;
	operator struct json_object *() const;

	type_t type() const { return _type; }

	const std::string toString() const {
		std::ostringstream oss;
		oss << "Option <" << key() << ">=<type=" << type() << ">=<val=";
		switch (type()) {
		case type_boolean:
			oss << (bool)(*this);
			break;
		case type_double:
			oss << (double)(*this);
			break;
		case type_int:
			oss << (int)(*this);
			break;
		case type_string:
			oss << (const char *)(*this);
			break;
		case type_object:
			oss << "json object";
			break; // TODO add operators
		case type_array:
			oss << "json array";
			break; // TODO add operators
		}
		oss << ">";
		return oss.str();
	}

  private:
	Option(const char *key);

	Option &operator=(const Option &); // assignment op.

	std::string _key;
	type_t _type;

	std::string _value_string;

	union {
		struct json_object
			*jso; // if the passed object was an array/object the array/object is kept here
		const char *string;
		int integer;
		double floating;
		int boolean : 1;
	} value;
};

inline std::ostream &operator<<(std::ostream &s, const Option &l) {
	s << l.toString();
	return s;
}

class OptionList { //: public List<Option> {

  public:
	typedef std::list<Option>::iterator iterator;
	typedef std::list<Option>::const_iterator const_iterator;

	const Option &lookup(std::list<Option> const &options, const std::string &key) const;
	const char *lookup_string(std::list<Option> const &options, const char *key) const;
	const char *lookup_string_tolower(std::list<Option> const &options, const char *key) const;
	int lookup_int(std::list<Option> const &options, const char *key) const;
	bool lookup_bool(std::list<Option> const &options, const char *key) const;
	double lookup_double(std::list<Option> const &options, const char *key) const;
	struct json_object *lookup_json_array(std::list<Option> const &options, const char *key) const;
	struct json_object *lookup_json_object(std::list<Option> const &options, const char *key) const;
	void dump(std::list<Option> const &options);

	void parse();

  protected:
};

#endif /* _OPTIONS_H_ */
