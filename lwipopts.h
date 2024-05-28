#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

#include "lwipopts_examples_common.h"

/* TCP WND must be at least 16 kb to match TLS record size
   or you will get a warning "altcp_tls: TCP_WND is smaller than the RX decrypion buffer, connection RX might stall!" */
#undef TCP_WND
#define TCP_WND  16384

#define LWIP_ALTCP               1
#define LWIP_DEBUG 1

// Two different modes:
#define PICO_CYW43_ARCH_THREADSAFE_BACKGROUND 1
#undef PICO_CYW43_ARCH_POLL

#endif

