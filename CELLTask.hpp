#ifndef _CELL_TASK_H_
#define _CELL_TASK_H_

#include <thread>
#include <mutex>
#include <list>
#include <functional>


//执行任务分服务类型
class CellTaskServer
{
    typedef std::function<void()> CellTask;
private:
    //任务数据
    std::list<CellTask> _tasks;
    //任务数据缓冲区
    std::list<CellTask> _tasksBuf;
    //改变数据缓冲区时需要枷锁
    std::mutex _mutex;

public:

    void addTask(CellTask task)
    {
        // _mutex.lock();
        std::lock_guard<std::mutex> lock(_mutex);
        _tasksBuf.push_back(task);
        // _mutex.unlock();
    }

    //启动服务
    void Start()
    {
        //线程
        std::thread t(std::mem_fn(&CellTaskServer::OnRun), this);
        t.detach();
    }
protected:
    void OnRun()
    {   
        while(true)
        {
            if(!_tasksBuf.empty())
            {
                std::lock_guard<std::mutex> lock(_mutex);
                for(auto pTask: _tasksBuf)
                {
                    _tasks.push_back(pTask);
                }
                _tasksBuf.clear();
            }
            //如果没有任务
            if(_tasks.empty())
            {
                std::chrono::milliseconds t(1);
                std::this_thread::sleep_for(t);
                continue;
            }

            for(auto pTask : _tasks)
            {
                pTask();
            }

            _tasks.clear();
        }
       
    }

};

#endif