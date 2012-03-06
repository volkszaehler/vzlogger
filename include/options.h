#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <list>

//#include "list.h"

class Option {

public:
	/* subset of json_type's */
	enum {
		type_boolean = 1,
		type_double,
		type_int,
		type_string = 6
	} type;

  //Option(const vz::Option &v);
	Option(char *key, char *value);
	Option(char *key, int value);
	Option(char *key, double value);
	Option(char *key, bool value);

	virtual ~Option();

  const char *key() const { return _key; }
	operator const char *();
	operator int();
	operator double();
	operator bool();

protected:
	Option(const char *key);

	char *_key;

	union {
		const char *string;
		int integer;
		double floating;
		int boolean:1;
	} value;

};


class OptionList { //: public List<Option> {
  
public:
  typedef std::list<Option>::iterator iterator;
  typedef std::list<Option>::const_iterator const_iterator;

  const Option& lookup(std::list<Option> options, char *key);
	void parse();

protected:

};


#endif /* _OPTIONS_H_ */
