#include "dlz_mysqlmm.h"



isc_result_t dlz_lookup(const char *zone, const char *name, void *dbdata, dns_sdlzlookup_t *lookup, dns_clientinfomethods_t *methods, dns_clientinfo_t *clientinfo)
{

}

isc_result_t dlz_findzonedb(void *dbdata, const char *name, dns_clientinfomethods_t *methods, dns_clientinfo_t *clientinfo)
{

}

isc_result_t dlz_allowzonexfr(void *dbdata, const char *name, const char *client)
{

}

isc_result_t dlz_allnodes(const char *zone, void *dbdata, dns_sdlzallnodes_t *allnodes)
{

}

isc_result_t dlz_authority(const char *zone, void *dbdata, dns_sdlzlookup_t *lookup)
{

}

isc_result_t dlz_newversion(const char *zone, void *dbdata, void **versionp)
{
	return ISC_R_NOTIMPLEMENTED;
}

void dlz_closeversion(const char *zone, isc_boolean_t commit, void *dbdata, void **versionp)
{
	if(versionp)
		*versionp = nullptr;

	return;
}

isc_result_t dlz_configure(dns_view_t *view, dns_dlzdb_t *dlzdb, void *dbdata)
{
	return ISC_R_NOTIMPLEMENTED;
}

isc_boolean_t dlz_ssumatch(const char *signer, const char *name, const char *tcpaddr, const char *type, const char *key, isc_uint32_t keydatalen, unsigned char *keydata, void *dbdata)
{
	return ISC_FALSE;
}

isc_result_t dlz_addrdataset(const char *name, const char *rdatastr, void *dbdata, void *version)
{
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t dlz_subrdataset(const char *name, const char *rdatastr, void *dbdata, void *version)
{
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t dlz_delrdataset(const char *name, const char *type, void *dbdata, void *version)
{
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t dlz_create(const char *dlzname, unsigned int argc, char *argv[], void **dbdata)
{

}

void dlz_destroy(void *dbdata)
{

}

int dlz_version(unsigned int *flags)
{
	*flags |= DNS_SDLZFLAG_RELATIVEOWNER |
	          DNS_SDLZFLAG_RELATIVERDATA |
	          DNS_SDLZFLAG_THREADSAFE;
	return DLZ_DLOPEN_VERSION;
}
