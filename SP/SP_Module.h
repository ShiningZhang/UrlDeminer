#ifndef SP_MODULE_H
#define SP_MODULE_H

#pragma once

#include <mutex>
#include <vector>
#include <thread>

#include "SP_Singleton.h"

class SP_Message_Queue;
class SP_Message_Block_Base;

class SP_Module
{
public:
    SP_Module();
    virtual ~SP_Module();
    virtual int open();
    virtual int close();
    virtual void activate(int n_threads = 1);
    virtual void wait();
    virtual void svc();
    static  void svc_run(void *);
    virtual int put(SP_Message_Block_Base *);
    virtual int get(SP_Message_Block_Base *&);
    void next(SP_Module *);
    SP_Module *next();
    virtual int put_next(SP_Message_Block_Base *);

private:
    std::mutex mutex_;
    std::mutex thread_mutex_;
    std::vector<std::thread> thread_pool;
    size_t thr_count_;
    bool terminated_;
    SP_Module *next_;
    SP_Message_Queue * msg_queue_;
};

#endif
