#include <exception>
#include <fstream>
#include <thread>
#include <mutex>

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

			QueryTypes qtype = MM_QUERY_ALLNODES;

			query.sql = queryval.get("sql", "").asString();

			if(queryname == "allnodes")
			{
				qtype = MM_QUERY_ALLNODES;
			}
			else if(queryname == "lookup")
			{
				qtype = MM_QUERY_LOOKUP;
			}
			else if(queryname == "findzone")
			{
				qtype = MM_QUERY_FINDZONE;
			}
			else if(queryname == "authority")
			{
				qtype = MM_QUERY_AUTHORITY;
			}
			else if(queryname == "allowxfr")
			{
				qtype = MM_QUERY_ALLOWXFR;
			}
			else if(queryname == "countzone")
			{
				qtype = MM_QUERY_COUNTZONE;
			}
			else
			{
				std::string msg = "MySQLMM: Unknown query name given: ";
				throw std::runtime_error(msg + queryname);
			}

			Json::Value paramsArr = queryval["params"];

			if(paramsArr.isArray())
			{
				for(const Json::Value &param: paramsArr)
				{
					std::string pstr = param.asString();

					if(pstr == "zone")
					{
						query.params.push_back(MM_PARAM_ZONE);
					}
					else if(pstr == "record")
					{
						query.params.push_back(MM_PARAM_RECORD);
					}
					else if(pstr == "client")
					{
						query.params.push_back(MM_PARAM_CLIENT);
					}
					else
					{
						std::string msg = "MySQLMM: Unknown query parameter: ";
						throw std::runtime_error(msg + pstr);
					}
				}
			}

			queries[qtype] = std::move(query);
		}
	}
	catch(const std::exception &e)
	{
		std::string msg = "Failed reading MySQLMM configuration: ";
		throw std::runtime_error(msg + e.what());
	}
}

static std::recursive_mutex connectionAquisitionMutex;

std::shared_ptr<MySQLMMManager::mmconn> MySQLMMManager::spawnConnection()
{
	std::lock_guard<std::recursive_mutex> lock(connectionAquisitionMutex);

	if(connections.size() >= max_connections)
		throw std::runtime_error("MySQLMM: Maximum number of allowed connections reached");

	std::shared_ptr<mmconn> res = std::make_shared<mmconn>();

	res->connection = std::shared_ptr<sql::Connection>(driver->connect(url, user, password));

	if(!db.empty())
		res->connection->setSchema(db);

	connections.push_back(res);

	return res;
}

std::shared_ptr<MySQLMMManager::mmconn> MySQLMMManager::getFreeConnection()
{
	std::lock_guard<std::recursive_mutex> lock(connectionAquisitionMutex);

	for(std::shared_ptr<mmconn> &conn: connections)
	{
		if(conn.unique())
		{
			return conn;
		}
	}

	return spawnConnection();
}
