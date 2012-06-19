#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>


#ifdef HAVE_CONFIG_HPP
#  include "config.hpp" /* GNU buildsystem config */
#else
#  include "../config.h" /* GNU buildsystem config */
#endif

#include <VZException.hpp>
#include <shared_ptr.hpp>


/* enumerations */
typedef enum {
	log_error = -1,
	log_warning = 0,
	log_info = 5,
	log_debug = 10,
	log_finest = 15
} log_level_t;

/* types */

/* constants */
#define SUCCESS 0
#define ERR -1
#define ERR_NOT_FOUND -2
#define ERR_INVALID_TYPE -3

/* prototypes */
void print(log_level_t lvl, const char *format, const char *id, ... );

#endif /* _COMMON_H_ */
