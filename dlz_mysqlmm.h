#ifndef H_DLZ_MYSQLMM__H
#define H_DLZ_MYSQLMM__H

#include <dns/dlz.h>
#include <dns/dlz_dlopen.h>

typedef void log_t(int level, const char *fmt, ...);

struct bind9_functions
{
	log_t *log;
	dns_sdlz_putrr_t *putrr;
	dns_sdlz_putnamedrr_t *putnamedrr;
	dns_dlz_writeablezone_t *writeable_zone;
};

extern "C"
{

#if defined _WIN32 || defined __CYGWIN__
# define EXPORT __declspec(dllexport)
#else
# if __GNUC__ >= 4
#  define EXPORT __attribute__((visibility("default")))
# else
#  define EXPORT
# endif
#endif

EXPORT isc_result_t dlz_lookup(const char *zone,
                        const char *name,
                        void *dbdata,
                        dns_sdlzlookup_t *lookup,
                        dns_clientinfomethods_t *methods,
                        dns_clientinfo_t *clientinfo);

EXPORT isc_result_t dlz_findzonedb(void *dbdata,
                            const char *name,
                            dns_clientinfomethods_t *methods,
                            dns_clientinfo_t *clientinfo);

EXPORT isc_result_t dlz_allowzonexfr(void *dbdata,
                              const char *name,
                              const char *client);

EXPORT isc_result_t dlz_allnodes(const char *zone,
                          void *dbdata,
                          dns_sdlzallnodes_t *allnodes);

EXPORT isc_result_t dlz_authority(const char *zone,
                           void *dbdata,
                           dns_sdlzlookup_t *lookup);

EXPORT isc_result_t dlz_newversion(const char *zone,
                            void *dbdata,
                            void **versionp);

EXPORT void dlz_closeversion(const char *zone,
                      isc_boolean_t commit,
                      void *dbdata,
                      void **versionp);

EXPORT isc_result_t dlz_configure(dns_view_t *view,
                           dns_dlzdb_t *dlzdb,
                           void *dbdata);

EXPORT isc_boolean_t dlz_ssumatch(const char *signer,
                           const char *name,
                           const char *tcpaddr,
                           const char *type,
                           const char *key,
                           isc_uint32_t keydatalen,
                           unsigned char *keydata,
                           void *dbdata);

EXPORT isc_result_t dlz_addrdataset(const char *name,
                             const char *rdatastr,
                             void *dbdata,
                             void *version);

EXPORT isc_result_t dlz_subrdataset(const char *name,
                             const char *rdatastr,
                             void *dbdata,
                             void *version);

EXPORT isc_result_t dlz_delrdataset(const char *name,
                             const char *type,
                             void *dbdata,
                             void *version);

EXPORT isc_result_t dlz_create(const char *dlzname,
                        unsigned int argc,
                        char *argv[],
                        void **dbdata,
                        ...);

EXPORT void dlz_destroy(void *dbdata);

EXPORT int dlz_version(unsigned int *flags);

}

#endif
