#include <exception>

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
		drv->threadInit();
	}

	void init_mt()
	{
		_init = true;
	}
};

static thread_local mysqlmm_thread_init mysqlmm_thread_init_obj;

MySQLMMManager *MySQLMMManager::inst = nullptr;

MySQLMMManager::MySQLMMManager(const std::string& dlzname,
                               const bind9_functions& b9f,
                               const std::vector<std::string>& args)
{
	if(inst)
		throw std::runtime_error("Tried to create more than one MySQLMMManager");

	mysqlmm_thread_init_obj.init_mt();

	inst = this;
}

MySQLMMManager::~MySQLMMManager()
{
	inst = nullptr;
}
