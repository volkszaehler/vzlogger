#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_HPP
#include "config.hpp" /* GNU buildsystem config */
#else
#include "../config.h" /* GNU buildsystem config */
#endif

#include <VZException.hpp>
#include <shared_ptr.hpp>

/* enumerations */
typedef enum {
	log_alert = 0,
	log_error = 1,
	log_warning = 3,
	log_info = 5,
	log_debug = 10,
	log_finest = 15
} log_level_t;

typedef enum { parity_8n1, parity_7n1, parity_7e1, parity_7o1 } parity_type_t;
/* types */

/* constants */
#define SUCCESS 0
#define ERR -1
#define ERR_NOT_FOUND -2
#define ERR_INVALID_TYPE -3

/* prototypes */
void print(log_level_t lvl, const char *format, const char *id, ...);

#endif /* _COMMON_H_ */
