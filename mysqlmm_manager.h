#pragma once

#include <vector>
#include <string>

struct bind9_functions;

class MySQLMMManager
{
	MySQLMMManager(const MySQLMMManager&) = delete;
	MySQLMMManager& operator=(const MySQLMMManager&) = delete;

	static MySQLMMManager *inst;

	public:
	MySQLMMManager(const std::string &dlzname,
	               const bind9_functions &b9f,
	               const std::vector<std::string> &args);
	~MySQLMMManager();
};
