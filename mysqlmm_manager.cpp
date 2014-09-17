#include <exception>
#include <fstream>

#include <json/json.h>
#include <mysql_driver.h>

#include "util.h"
#include "mysqlmm_manager.h"

class mysqlmm_thread_init
{
	bool _init;
	sql::Driver *_drv;

	public:
	mysqlmm_thread_init()
		:_init(false)
		,_drv(nullptr)
	{}

	~mysqlmm_thread_init()
	{
		clear();
	}

	void clear()
	{
		if(_init && _drv)
		{
			_drv->threadEnd();
		}

		_init = false;
		_drv = nullptr;
	}

	void init(MySQLMMManager *mm)
	{
		if(_init)
			return;

		_init = true;
		_drv = mm->driver;
		_drv->threadInit();
	}
};

static thread_local mysqlmm_thread_init mysqlmm_thread_init_obj;

MySQLMMManager::MySQLMMManager(const std::string& dlzname,
                               const bind9_functions& b9f,
                               const std::vector<std::string>& args)
	:f(b9f)
	,initial_connections(1)
	,max_connections(1)
{
	if(args.size() != 2)
		throw std::runtime_error("MySQLMM expects exactly one argument");

	f.log(ISC_LOG_INFO, "MySQLMM driver instance %s starting", dlzname.c_str());

	driver = sql::mysql::get_driver_instance();

	mysqlmm_thread_init_obj.init(this);
}

MySQLMMManager::~MySQLMMManager()
{
}

void MySQLMMManager::readConfig(const std::string &cfg)
{
	try
	{
		Json::Reader reader;
		Json::Value root;

		std::ifstream ifs;
		ifs.open(cfg);

		if(!reader.parse(ifs, root))
			throw std::runtime_error("Failed parsing MySQLMM config");

		url = root.get("url", "tcp://localhost/bind").asString();
		user = root.get("user", "bind").asString();
		password = root.get("pass", "").asString();
		db = root.get("db", "").asString();
		initial_connections = root.get("initial_connections", 1).asUInt();
		max_connections = root.get("max_connections", initial_connections + 8).asUInt();

		Json::Value queriesObj = root["queries"];

		for(const std::string &queryname: queriesObj.getMemberNames())
		{
			Json::Value queryval = queriesObj[queryname];

			mmquery query;

			query.sql = queryval.get("sql", "").asString();

			Json::Value paramsArr = queryval["params"];

			if(paramsArr.isArray())
			{
				for(const Json::Value &param: paramsArr)
				{
					query.params.push_back(param.asString());
				}
			}

			queries[queryname] = std::move(query);
		}
	}
	catch(const std::exception &e)
	{
		std::string msg = "Failed reading MySQLMM configuration: ";
		throw std::runtime_error(msg + e.what());
	}
}

std::shared_ptr<sql::Connection> MySQLMMManager::spawnConnection()
{
	std::shared_ptr<sql::Connection> res;
	return res;
}

std::shared_ptr<sql::Connection> MySQLMMManager::getFreeConnection()
{
	std::shared_ptr<sql::Connection> res;
	return res;
}
