#include "zookeeperutil.h"
#include "logger.h"
//#define THREADED

// 全局的watcher观察器   zkserver给zkclient的通知
// watcher线程  
void global_watcher(zhandle_t *zh, int type,
                    int state, const char *path, void *watcherCtx)
{
    //链接成功了 会下发回调 查看type 和state 改变信号量
    if(type == ZOO_SESSION_EVENT)
    {
        if(state == ZOO_CONNECTED_STATE)
        {
            // zkclient和zkserver连接成功
            sem_t *sem = (sem_t*)zoo_get_context(zh);
            sem_post(sem);
        }
    }
    else if (type == ZOO_CREATED_EVENT)
    {
        LOG_INFO("ZooKeeper node created");
    }
    else if (type == ZOO_DELETED_EVENT)
    {
        LOG_INFO("ZooKeeper node deleted: ");
    }
    else if (type == ZOO_CHANGED_EVENT)
    {
        LOG_INFO("ZooKeeper node changed");
    }

}


//初始化
ZkClient::ZkClient() : _zhandle(nullptr)
{
}

//关闭
ZkClient::~ZkClient()
{
    if (_zhandle != nullptr)
    {
        zookeeper_close(_zhandle);
    }
}

// 重连
void ZkClient::Reconnect(){
    if(_zhandle!=nullptr){
        zookeeper_close(_zhandle);
    }
    Start();
}

// zkclient启动连接zkserver
void ZkClient::Start()
{
    std::string host = MprpcApplication::GetConfig().LoadConfig("zookeeperip");
    std::string port = MprpcApplication::GetConfig().LoadConfig("zookeeperport");
    std::string constr = host + ":" + port;

    // 创建句柄
    _zhandle = zookeeper_init(constr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);
    if (nullptr == _zhandle)
    {
        LOG_ERROR("zookeeper_init error!");
    }
    // 信号量 等待连接
    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(_zhandle, &sem);

    //最多等待10秒
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 10;

    if(sem_timedwait(&sem, &ts) == -1){
        LOG_ERROR("zookeeper_init timeout!");
        zookeeper_close(_zhandle);
        _zhandle = nullptr;
        return;
    }  
    LOG_INFO("zookeeper_init success!");
}


// 在zkserver上根据指定的path创建znode节点
void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    //确保zk在线
    if(_zhandle == nullptr){
        LOG_ERROR("zkclient obj not inited");
        return;
    }
    char pathBuf[128];
    int Buflen = sizeof(pathBuf);
    int flag;
    flag = zoo_exists(_zhandle, path, 0, nullptr);
    LOG_INFO("znode is going to create...");
    //std::cout << "znode create"<< flag << std::endl;
    if(ZNONODE == flag) // 表示path的znode节点不存在
    {
        // 创建指定path的znode节点了
        //开放权限（所有用户可操作）
        flag = zoo_create(_zhandle, path, data, datalen, &ZOO_OPEN_ACL_UNSAFE, state, pathBuf, Buflen);
        if(ZOK == flag)
        {
            LOG_INFO("zookeeper_node success!");
            //std::cout << "znode create success... path:" << path << std::endl;
        }
        else
		{
			//std::cout << "flag:" << flag << std::endl;
			//std::cout << "znode create error... path:" << path << std::endl;
            LOG_ERROR("zookeeper_node error!");
			//exit(EXIT_FAILURE);
		}
    }
    std::cout << "znode create"<< flag << std::endl;
    return;

}
// 根据参数指定的znode节点路径，获取znode节点的值
std::string ZkClient::GetData(const char *path)
{
    if(_zhandle==nullptr){
        LOG_ERROR("zookeeper_init error!");
        return "";
    }
    char buffer[64] ={0};
    int Buflen = sizeof(buffer);
    int flag = zoo_get(_zhandle, path, 0, buffer, &Buflen, nullptr);
    if (flag != ZOK)
	{
        LOG_ERROR("zoo_get error!");
		//std::cout << "get znode error... path:" << path << std::endl;
		return "";
	}
	else
	{
		return buffer;
	}
}