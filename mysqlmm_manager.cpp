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

	void init(sql::Driver *drv)
	{
		if(_init)
			return;

		_init = true;
		_drv = drv;
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

	readConfig(args[1]);

	driver = sql::mysql::get_driver_instance();

	mysqlmm_thread_init_obj.init(driver);

	for(unsigned int i = 0; i < initial_connections; ++i)
		spawnConnection();
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

		if(max_connections < initial_connections)
			throw std::runtime_error("max_connections must be >= initial_connections");

		Json::Value queriesObj = root["queries"];

		bool have_findzone = false;
		bool have_lookup = false;

		for(std::string queryname: queriesObj.getMemberNames())
		{
			Json::Value queryval = queriesObj[queryname];

			strtolower(queryname);

			mmquery query;

			QueryTypes qtype = MM_QUERY_ALLNODES;

			query.sql = queryval.get("sql", "").asString();

			bool need_client = false;
			bool need_zone = false;
			bool need_record = false;

			if(queryname == "allnodes")
			{
				qtype = MM_QUERY_ALLNODES;
				need_zone = true;
			}
			else if(queryname == "lookup")
			{
				qtype = MM_QUERY_LOOKUP;
				have_lookup = true;
				need_record = true;
			}
			else if(queryname == "findzone")
			{
				qtype = MM_QUERY_FINDZONE;
				have_findzone = true;
				need_zone = true;
			}
			else if(queryname == "authority")
			{
				qtype = MM_QUERY_AUTHORITY;
				need_zone = true;
			}
			else if(queryname == "allowxfr")
			{
				qtype = MM_QUERY_ALLOWXFR;
				need_zone = true;
				need_client = true;
			}
			else if(queryname == "countzone")
			{
				qtype = MM_QUERY_COUNTZONE;
				need_zone = true;
			}
			else
			{
				std::string msg = "MySQLMM: Unknown query name given: ";
				throw std::runtime_error(msg + queryname);
			}

			Json::Value paramsArr = queryval["params"];

			bool have_zone = false;
			bool have_record = false;
			bool have_client = false;

			if(paramsArr.isArray())
			{
				for(const Json::Value &param: paramsArr)
				{
					std::string pstr = param.asString();

					if(pstr == "zone")
					{
						query.params.push_back(MM_PARAM_ZONE);
						have_zone = true;
					}
					else if(pstr == "record")
					{
						query.params.push_back(MM_PARAM_RECORD);
						have_record = true;
					}
					else if(pstr == "client")
					{
						query.params.push_back(MM_PARAM_CLIENT);
						have_client = true;
					}
					else
					{
						std::string msg = "MySQLMM: Unknown query parameter: ";
						throw std::runtime_error(msg + pstr);
					}
				}
			}

			if( (need_zone   && !have_zone  )
			 || (need_record && !have_record)
			 || (need_client && !have_client))
			{
				f.log(ISC_LOG_INFO, "%s query needs %s%s%s parameters",
				      queryname.c_str(),
				      need_zone ? "zone " : "",
				      need_record ? "record " : "",
				      need_client ? "client " : "");
				throw std::runtime_error("Required parameters not present");
			}

			queries[qtype] = std::move(query);
		}

		if(!have_findzone || !have_lookup)
		{
			throw std::runtime_error("findzone and lookup queries are required");
		}
	}
	catch(const std::exception &e)
	{
		queries.clear();
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

bool MySQLMMManager::findzonedb(const std::string& name)
{
	return true;
}

bool MySQLMMManager::lookup(const std::string& zone, const std::string& name, dns_sdlzlookup_t* lookup)
{
	f.putrr(lookup, "A", 86400, "1.2.3.4");

	return true;
}
