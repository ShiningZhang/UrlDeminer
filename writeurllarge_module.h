#ifndef WRITEURLLARGE_MODULE_H_
#define WRITEURLLARGE_MODULE_H_
#pragma once

#include "SP_Module.h"

#include <mutex>

class WriteUrlLarge_Module : public SP_Module
{
public:
    static WriteUrlLarge_Module * instance()
    {
        return SP_Singleton<WriteUrlLarge_Module>::instance();
    }
    WriteUrlLarge_Module(int threads = 1);
    virtual ~WriteUrlLarge_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};
#endif