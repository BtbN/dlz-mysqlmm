#pragma once

#include <dns/dlz_dlopen.h>

typedef void log_t(int level, const char *fmt, ...);

struct bind9_functions
{
	log_t *log;
	dns_sdlz_putrr_t *putrr;
	dns_sdlz_putnamedrr_t *putnamedrr;
	dns_dlz_writeablezone_t *writeable_zone;

};

#define MM_UNUSED(x) ((void)x)
