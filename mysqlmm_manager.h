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
}

class MySQLMMManager
{
	friend class mysqlmm_thread_init;

	struct mmquery
	{
		std::string sql;
		std::vector<std::string> params;
		std::shared_ptr<sql::PreparedStatement> prep_stmt;
	};

	struct mmconn
	{
		std::shared_ptr<sql::Connection> connection;
		std::unordered_map<std::string, mmquery> queries;
	};

	MySQLMMManager(const MySQLMMManager&) = delete;
	MySQLMMManager& operator=(const MySQLMMManager&) = delete;

	public:
	MySQLMMManager(const std::string &dlzname,
	               const bind9_functions &b9f,
	               const std::vector<std::string> &args);
	~MySQLMMManager();

	private:
	void readConfig(const std::string &cfg);
	std::shared_ptr<sql::Connection> spawnConnection();
	std::shared_ptr<sql::Connection> getFreeConnection();

	private:
	sql::mysql::MySQL_Driver *driver;
	std::vector<std::shared_ptr<mmconn>> connections;
	bind9_functions f;

	std::string url;
	std::string user;
	std::string password;
	std::string db;
	unsigned int initial_connections;
	unsigned int max_connections;
	std::unordered_map<std::string, mmquery> queries;
};
