#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include "list.h"

typedef union {
	const char *string;
	int integer;
	double floating;
	int boolean:1;
} option_value_t;

/* subset of json_type's */
typedef enum {
	option_type_boolean = 1,
	option_type_double,
	option_type_int,
	option_type_string = 6
} option_type_t;

typedef struct {
	char *key;
	option_type_t type;
	option_value_t value;
} option_t;

/**
 * Lookup option by key in a list of options
 *
 * @param list_t the list of options
 * @param char *key the key you are looking for
 * @return int success or error (CFG_* constants)
 */
int options_lookup(list_t options, char *key, void *value, option_type_t type);

/**
 * Type specific wrapper functions for config_lookup_type()
 */
int options_lookup_string(list_t options, char *key, char **value);
int options_lookup_int(list_t options, char *key, int *value);
int options_lookup_double(list_t options, char *key, double *value);
int options_lookup_boolean(list_t options, char *key, int *value);

#endif /* _OPTIONS_H_ */
