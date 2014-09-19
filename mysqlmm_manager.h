#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include "util.h"

namespace sql
{
	namespace mysql
	{
		class MySQL_Driver;
	}
	class Connection;
	class PreparedStatement;
	class ResultSet;
}

class MySQLMMManager
{
	friend class mysqlmm_thread_init;

	enum QueryTypes
	{
		MM_QUERY_ALLNODES,
		MM_QUERY_LOOKUP,
		MM_QUERY_FINDZONE,
		MM_QUERY_AUTHORITY,
		MM_QUERY_ALLOWXFR,
		MM_QUERY_COUNTZONE
	};

	enum QueryParams
	{
		MM_PARAM_ZONE,
		MM_PARAM_RECORD,
		MM_PARAM_CLIENT
	};

	struct mmquery
	{
		std::string sql;
		std::vector<QueryParams> params;
		std::shared_ptr<sql::PreparedStatement> prep_stmt;
	};

	struct mmconn
	{
		std::shared_ptr<sql::Connection> connection;
		std::unordered_map<QueryTypes, mmquery, std::hash<int>> queries;
	};

	MySQLMMManager(const MySQLMMManager&) = delete;
	MySQLMMManager& operator=(const MySQLMMManager&) = delete;

	public:
	MySQLMMManager(const std::string &dlzname,
	               const bind9_functions &b9f,
	               const std::vector<std::string> &args);
	~MySQLMMManager();

	bool findzonedb(const std::string &zone);
	void lookup(const std::string &zone, const std::string &name, dns_sdlzlookup_t *lookup);

	inline bool hasAuthority() { return queries.find(MM_QUERY_AUTHORITY) != queries.end(); }
	void authority(const std::string &zone, dns_sdlzlookup_t *lookup);

	inline bool hasAllnodes() { return queries.find(MM_QUERY_ALLNODES) != queries.end(); }
	void allnodes(const std::string &zone, dns_sdlzallnodes_t *allnodes);

	inline bool hasAllowxfr() { return queries.find(MM_QUERY_ALLOWXFR) != queries.end(); }
	bool allowxfr(const std::string &zone, const std::string &client);

	inline bool hasCountzone() { return queries.find(MM_QUERY_COUNTZONE) != queries.end(); }
	void countzone(const std::string &zone);

	private:
	void readConfig(const std::string &cfg);
	std::shared_ptr<mmconn> spawnConnection();
	std::shared_ptr<mmconn> getFreeConnection();
	void fillPrepQry(mmquery &qry, const std::string &zone = "", const std::string &record = "", const std::string &client = "");
	void process_look_auth_res(dns_sdlzlookup_t* lookup, const std::unique_ptr<sql::ResultSet> &res);

	public:
	bind9_functions f;

	private:
	sql::mysql::MySQL_Driver *driver;
	std::vector<std::shared_ptr<mmconn>> connections;

	std::string url;
	std::string user;
	std::string password;
	std::string db;
	unsigned int initial_connections;
	unsigned int max_connections;
	std::unordered_map<QueryTypes, mmquery, std::hash<int>> queries;
};
