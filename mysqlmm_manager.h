#pragma once

#include <vector>
#include <string>
#include <memory>

#include "util.h"

namespace sql
{
	namespace mysql
	{
		class MySQL_Driver;
	}
	class Connection;
}

class MySQLMMManager
{
	friend class mysqlmm_thread_init;

	MySQLMMManager(const MySQLMMManager&) = delete;
	MySQLMMManager& operator=(const MySQLMMManager&) = delete;

	static MySQLMMManager *inst;

	public:
	MySQLMMManager(const std::string &dlzname,
	               const bind9_functions &b9f,
	               const std::vector<std::string> &args);
	~MySQLMMManager();

	private:
	std::shared_ptr<sql::Connection> spawnConnection();
	std::shared_ptr<sql::Connection> getFreeConnection();

	private:
	sql::mysql::MySQL_Driver *driver;
	std::vector<std::shared_ptr<sql::Connection>> connections;
	bind9_functions f;
};
