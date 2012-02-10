#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include "list.h"

class Option {

public:
	Option(char *key, char *value);
	Option(char *key, int value);
	Option(char *key, double value);
	Option(char *key, bool value);

	virtual ~Option();

	operator (char *)();
	operator int();
	operator double();
	operator bool();

protected:
	Option(char *key);

	char *key;

	union {
		const char *string;
		int integer;
		double floating;
		int boolean:1;
	} value;

	/* subset of json_type's */
	enum {
		type_boolean = 1,
		type_double,
		type_int,
		type_string = 6
	} type;
}

class OptionList : public List<Option> {

public:
	Option& lookup(char *key);
	void parse();

protected:

}

#endif /* _OPTIONS_H_ */
