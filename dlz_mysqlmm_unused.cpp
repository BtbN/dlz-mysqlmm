#include "dlz_mysqlmm.h"
#include "util.h"



isc_result_t dlz_configure(dns_view_t *view,
                           dns_dlzdb_t *dlzdb,
                           void *dbdata)
{
	MM_UNUSED(view);
	MM_UNUSED(dlzdb);
	MM_UNUSED(dbdata);

	return ISC_R_SUCCESS;
}

isc_boolean_t dlz_ssumatch(const char *signer,
                           const char *name,
                           const char *tcpaddr,
                           const char *type,
                           const char *key,
                           isc_uint32_t keydatalen,
                           unsigned char *keydata,
                           void *dbdata)
{
	MM_UNUSED(signer);
	MM_UNUSED(name);
	MM_UNUSED(tcpaddr);
	MM_UNUSED(type);
	MM_UNUSED(key);
	MM_UNUSED(keydatalen);
	MM_UNUSED(keydata);
	MM_UNUSED(dbdata);

	return ISC_FALSE;
}

isc_result_t dlz_addrdataset(const char *name,
                             const char *rdatastr,
                             void *dbdata,
                             void *version)
{
	MM_UNUSED(name);
	MM_UNUSED(rdatastr);
	MM_UNUSED(dbdata);
	MM_UNUSED(version);

	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t dlz_subrdataset(const char *name,
                             const char *rdatastr,
                             void *dbdata,
                             void *version)
{
	MM_UNUSED(name);
	MM_UNUSED(rdatastr);
	MM_UNUSED(dbdata);
	MM_UNUSED(version);

	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t dlz_delrdataset(const char *name,
                             const char *type,
                             void *dbdata,
                             void *version)
{
	MM_UNUSED(name);
	MM_UNUSED(type);
	MM_UNUSED(dbdata);
	MM_UNUSED(version);

	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t dlz_newversion(const char *zone,
                            void *dbdata,
                            void **versionp)
{
	MM_UNUSED(zone);
	MM_UNUSED(dbdata);
	MM_UNUSED(versionp);

	return ISC_R_NOTIMPLEMENTED;
}
