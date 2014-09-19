#include <exception>
#include <sstream>
#include <fstream>
#include <thread>
#include <mutex>

#include <json/json.h>

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>

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

	f.log(ISC_LOG_INFO, "MySQLMM driver instance \"%s\" starting", dlzname.c_str());

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
			throw std::runtime_error(std::string("Failed parsing MySQLMM config: ") + reader.getFormatedErrorMessages());

		url = root.get("url", "tcp://localhost/bind").asString();
		user = root.get("user", "bind").asString();
		password = root.get("pass", "").asString();
		db = root.get("db", "").asString();
		initial_connections = root.get("initial_connections", 1).asUInt();
		max_connections = root.get("max_connections", initial_connections + 8).asUInt();

		if(max_connections < initial_connections)
			throw std::runtime_error("max_connections must be >= initial_connections");

		f.log(ISC_LOG_INFO, "MySQLMM Settings:");
		f.log(ISC_LOG_INFO, " - url = %s", url.c_str());
		f.log(ISC_LOG_INFO, " - user = %s", user.c_str());
		f.log(ISC_LOG_INFO, " - password %s", password.empty() ? "not set" : "set");
		f.log(ISC_LOG_INFO, " - db = %s", db.c_str());
		f.log(ISC_LOG_INFO, " - initial_connections = %d", initial_connections);
		f.log(ISC_LOG_INFO, " - max_connections = %d", max_connections);

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
				f.log(ISC_LOG_ERROR, "%s query needs %s%s%s parameters",
				      queryname.c_str(),
				      need_zone ? "zone " : "",
				      need_record ? "record " : "",
				      need_client ? "client " : "");
				throw std::runtime_error("Required parameters not present");
			}

			f.log(ISC_LOG_INFO, " - %s query with %d parameters: %s", queryname.c_str(), (int)query.params.size(), query.sql.c_str());

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

	res->queries = queries;

	for(auto &qry: res->queries)
	{
		qry.second.prep_stmt = std::shared_ptr<sql::PreparedStatement>(res->connection->prepareStatement(qry.second.sql));

		if(!qry.second.prep_stmt)
			throw std::runtime_error(std::string("Failed preparing query: ") + qry.second.sql);
	}

	connections.push_back(res);

	return res;
}

std::shared_ptr<MySQLMMManager::mmconn> MySQLMMManager::getFreeConnection()
{
	mysqlmm_thread_init_obj.init(driver);

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

void MySQLMMManager::fillPrepQry(MySQLMMManager::mmquery &qry, const std::string& zone, const std::string& record, const std::string& client)
{
	for(unsigned int i = 0; i < qry.params.size(); ++i)
	{
		switch(qry.params[i])
		{
			case MM_PARAM_CLIENT:
				qry.prep_stmt->setString(i + 1, client);
				break;
			case MM_PARAM_ZONE:
				qry.prep_stmt->setString(i + 1, zone);
				break;
			case MM_PARAM_RECORD:
				qry.prep_stmt->setString(i + 1, record);
				break;
		}
	}
}

void MySQLMMManager::process_look_auth_res(dns_sdlzlookup_t* lookup, const std::unique_ptr<sql::ResultSet>& res)
{
	sql::ResultSetMetaData *meta = res->getMetaData();

	if(!meta)
		throw std::runtime_error("lookup needs result metadata");

	unsigned int cols = meta->getColumnCount();

	while(res->next())
	{
		isc_result_t result;

		switch(cols)
		{
			case 0:
				throw std::runtime_error("Zero columns in result!");
				break;
			case 1:
				result = f.putrr(lookup,
				                 "A",
				                 86400,
				                 res->getString(1).c_str());
				f.log(ISC_LOG_INFO, "MySQLMM Result 1: A %s", res->getString(1).c_str());
				break;
			case 2:
				result = f.putrr(lookup,
				                 res->getString(1).c_str(),
				                 86400,
				                 res->getString(2).c_str());
				f.log(ISC_LOG_INFO, "MySQLMM Result 2: %s %s", res->getString(1).c_str(), res->getString(2).c_str());
				break;
			default:
			{
				std::ostringstream str;
				std::string sep = "";

				for(unsigned int i = 3; i <= cols; ++i)
				{
					std::string part = res->getString(i);

					if(!part.empty())
					{
						str << sep << part;
						sep = " ";
					}
				}

				result = f.putrr(lookup,
				                 res->getString(2).c_str(),
				                 res->getInt(1),
				                 str.str().c_str());
				f.log(ISC_LOG_INFO, "MySQLMM Result +: %s %s", res->getString(2).c_str(), str.str().c_str());
			}
		}

		if(result != ISC_R_SUCCESS)
			throw std::runtime_error("MySQLMM putrr failed");
	}
}

bool MySQLMMManager::findzonedb(const std::string& zone)
{
	std::shared_ptr<mmconn> con = getFreeConnection();

	mmquery &qry = con->queries.at(MM_QUERY_FINDZONE);
	fillPrepQry(qry, zone);

	std::unique_ptr<sql::ResultSet> res(qry.prep_stmt->executeQuery());

	if(res->next())
	{
		f.log(ISC_LOG_INFO, "MySQLMM Found zone %s!", zone.c_str());
		return true;
	}

	f.log(ISC_LOG_INFO, "MySQLMM Not found zone %s!", zone.c_str());
	return false;
}

void MySQLMMManager::lookup(const std::string& zone, const std::string& name, dns_sdlzlookup_t* lookup)
{
	std::shared_ptr<mmconn> con = getFreeConnection();

	mmquery &qry = con->queries.at(MM_QUERY_LOOKUP);
	fillPrepQry(qry, zone, name);

	std::unique_ptr<sql::ResultSet> res(qry.prep_stmt->executeQuery());

	f.log(ISC_LOG_INFO, "MySQLMM Looking for %s in zone %s!", name.c_str(), zone.c_str());

	process_look_auth_res(lookup, res);
}

void MySQLMMManager::authority(const std::string& zone, dns_sdlzlookup_t* lookup)
{
	std::shared_ptr<mmconn> con = getFreeConnection();

	mmquery &qry = con->queries.at(MM_QUERY_AUTHORITY);
	fillPrepQry(qry, zone);

	std::unique_ptr<sql::ResultSet> res(qry.prep_stmt->executeQuery());

	f.log(ISC_LOG_INFO, "MySQLMM Looking for authority of %s!", zone.c_str());

	process_look_auth_res(lookup, res);
}

void MySQLMMManager::allnodes(const std::string &zone, dns_sdlzallnodes_t *allnodes)
{
	std::shared_ptr<mmconn> con = getFreeConnection();

	mmquery &qry = con->queries.at(MM_QUERY_ALLNODES);
	fillPrepQry(qry, zone);

	std::unique_ptr<sql::ResultSet> res(qry.prep_stmt->executeQuery());
	sql::ResultSetMetaData *meta = res->getMetaData();

	if(!meta)
		throw std::runtime_error("MySQLMM allnodes needs result metadata");

	unsigned int cols = meta->getColumnCount();

	if(cols < 4)
		throw std::runtime_error("MySQLMM allnodes query returned less than 4 fields!");

	f.log(ISC_LOG_INFO, "MySQLMM Looking for allnodes of %s!", zone.c_str());

	while(res->next())
	{
		std::ostringstream str;
		std::string sep = "";

		for(unsigned int i = 4; i <= cols; ++i)
		{
			std::string part = res->getString(i);

			if(!part.empty())
			{
				str << sep << part;
				sep = " ";
			}
		}

		isc_result_t result = f.putnamedrr(allnodes,
		                                   res->getString(3).c_str(),
		                                   res->getString(2).c_str(),
		                                   res->getUInt(1),
		                                   str.str().c_str());

		if(result != ISC_R_SUCCESS)
			throw std::runtime_error("MySQLMM putnamedrr failed");

		f.log(ISC_LOG_INFO, "MySQLMM allnodes result: %s %s %d %s", res->getString(3).c_str(), res->getString(2).c_str(), res->getUInt(1), str.str().c_str());
	}
}

bool MySQLMMManager::allowxfr(const std::string &zone, const std::string &client)
{
	std::shared_ptr<mmconn> con = getFreeConnection();

	mmquery &qry = con->queries.at(MM_QUERY_ALLOWXFR);
	fillPrepQry(qry, zone, "", client);

	std::unique_ptr<sql::ResultSet> res(qry.prep_stmt->executeQuery());

	if(res->next())
	{
		f.log(ISC_LOG_INFO, "MySQLMM Client %s allowed xfr in zone %s!", client.c_str(), zone.c_str());
		return true;
	}

	f.log(ISC_LOG_INFO, "MySQLMM Client %s NOT allowed xfr in zone %s!", client.c_str(), zone.c_str());
	return false;
}

void MySQLMMManager::countzone(const std::string &zone)
{
	std::shared_ptr<mmconn> con = getFreeConnection();

	mmquery &qry = con->queries.at(MM_QUERY_COUNTZONE);
	fillPrepQry(qry, zone);

	int cnt = qry.prep_stmt->executeUpdate();

	f.log(ISC_LOG_INFO, "MySQL Updated zone count for zone %s. result %d", zone.c_str(), cnt);
}
