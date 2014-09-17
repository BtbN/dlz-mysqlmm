#include <exception>

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

MySQLMMManager *MySQLMMManager::inst = nullptr;

MySQLMMManager::MySQLMMManager(const std::string& dlzname,
                               const bind9_functions& b9f,
                               const std::vector<std::string>& args)
	:f(b9f)
{
	if(inst)
		throw std::runtime_error("Tried to create more than one MySQLMMManager");

	if(args.size() != 2)
		throw std::runtime_error("MySQLMM expects exactly one argument");

	f.log(ISC_LOG_INFO, "MySQLMM driver instance %s starting", dlzname.c_str());

	driver = sql::mysql::get_driver_instance();

	mysqlmm_thread_init_obj.init(this);
	inst = this;
}

MySQLMMManager::~MySQLMMManager()
{
	inst = nullptr;
}

std::shared_ptr<sql::Connection> MySQLMMManager::spawnConnection()
{

}

std::shared_ptr<sql::Connection> MySQLMMManager::getFreeConnection()
{

}
