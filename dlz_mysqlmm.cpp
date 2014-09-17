#include <exception>

#include "mysqlmm_manager.h"
#include "util.h"

#include "dlz_mysqlmm.h"


isc_result_t dlz_lookup(const char *zone,
                        const char *name,
                        void *dbdata,
                        dns_sdlzlookup_t *lookup,
                        dns_clientinfomethods_t *methods,
                        dns_clientinfo_t *clientinfo)
{
	return ISC_R_UNEXPECTED;
}

isc_result_t dlz_findzonedb(void *dbdata,
                            const char *name,
                            dns_clientinfomethods_t *methods,
                            dns_clientinfo_t *clientinfo)
{
	return ISC_R_UNEXPECTED;
}

isc_result_t dlz_allowzonexfr(void *dbdata,
                              const char *name,
                              const char *client)
{
	return ISC_R_UNEXPECTED;
}

isc_result_t dlz_allnodes(const char *zone,
                          void *dbdata,
                          dns_sdlzallnodes_t *allnodes)
{
	return ISC_R_UNEXPECTED;
}

isc_result_t dlz_authority(const char *zone,
                           void *dbdata,
                           dns_sdlzlookup_t *lookup)
{
	return ISC_R_UNEXPECTED;
}

void dlz_closeversion(const char *zone,
                      isc_boolean_t commit,
                      void *dbdata,
                      void **versionp)
{
	if(versionp)
		*versionp = nullptr;

	return;
}

isc_result_t dlz_create(const char *dlzname,
                        unsigned int argc,
                        char *argv[],
                        void **dbdata,
                        ...)
{
	va_list ap;
	const char *helper_name = nullptr;
	bind9_functions b9funcs = { 0, 0, 0, 0 };

	va_start(ap, dbdata);
	while(helper_name = va_arg(ap, decltype(helper_name)))
	{
		if(strcmp("log", helper_name) == 0)
		{
			b9funcs.log_f = va_arg(ap, decltype(b9funcs.log_f));
		}
		else if(strcmp("putrr", helper_name) == 0)
		{
			b9funcs.putrr_f = va_arg(ap, decltype(b9funcs.putrr_f));
		}
		else if(strcmp("putnamedrr", helper_name) == 0)
		{
			b9funcs.putnamedrr_f = va_arg(ap, decltype(b9funcs.putnamedrr_f));
		}
		else if(strcmp("writeable_zone", helper_name) == 0)
		{
			b9funcs.writeable_zone_f = va_arg(ap, decltype(b9funcs.writeable_zone_f));
		}
	}
	va_end(ap);

	std::vector<std::string> args;
	args.resize(argc);

	for(unsigned int i = 0; i < argc; ++i)
		args[i] = argv[i];

	try
	{
		MySQLMMManager *res = new MySQLMMManager(dlzname, b9funcs, args);
		*dbdata = res;
	}
	catch(const std::exception &e)
	{
		return ISC_R_UNEXPECTED;
	}

	return ISC_R_SUCCESS;
}

void dlz_destroy(void *dbdata)
{
	MySQLMMManager *mm = (MySQLMMManager*)dbdata;
	delete mm;
}

int dlz_version(unsigned int *flags)
{
	*flags |= DNS_SDLZFLAG_RELATIVEOWNER |
	          DNS_SDLZFLAG_RELATIVERDATA |
	          DNS_SDLZFLAG_THREADSAFE;
	return DLZ_DLOPEN_VERSION;
}
