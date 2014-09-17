#pragma once

#include <dns/dlz_dlopen.h>

#include <string>
#include <algorithm>

typedef void log_t(int level, const char *fmt, ...);

struct bind9_functions
{
	log_t *log_f;
	dns_sdlz_putrr_t *putrr_f;
	dns_sdlz_putnamedrr_t *putnamedrr_f;
	dns_dlz_writeablezone_t *writeable_zone_f;

	template<typename... Args>
	inline void log(int level, const char *fmt, Args... args)
	{
		if(!log_f)
			return;

		log_f(level, fmt, args...);
	}

	inline isc_result_t putrr(dns_sdlzlookup_t *lookup, const char *type, dns_ttl_t ttl, const char *data)
	{
		if(!putrr_f)
			return ISC_R_NOTIMPLEMENTED;

		return putrr_f(lookup, type, ttl, data);
	}

	inline isc_result_t putnamedrr(dns_sdlzallnodes_t *allnodes, const char *name, const char *type, dns_ttl_t ttl, const char *data)
	{
		if(!putnamedrr_f)
			return ISC_R_NOTIMPLEMENTED;

		return putnamedrr_f(allnodes, name, type, ttl, data);
	}

	inline isc_result_t writeable_zone(dns_view_t *view, dns_dlzdb_t *dlzdb, const char *zone_name)
	{
		if(!writeable_zone_f)
			return ISC_R_NOTIMPLEMENTED;

		return writeable_zone_f(view, dlzdb, zone_name);
	}
};

inline void strtolower(std::string &str)
{
	std::transform(str.begin(), str.end(), str.begin(), &::tolower);
}

inline void strtoupper(std::string &str)
{
	std::transform(str.begin(), str.end(), str.begin(), &::toupper);
}

#define MM_UNUSED(x) ((void)x)
