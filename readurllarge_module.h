#ifndef READURLLARGE_MODULE_H_
#define READURLLARGE_MODULE_H_
#pragma once

#include "SP_Module.h"

#include <mutex>

class ReadUrlLarge_Module : public SP_Module
{
public:
    static ReadUrlLarge_Module * instance()
    {
        return SP_Singleton<ReadUrlLarge_Module>::instance();
    }
    ReadUrlLarge_Module(int threads = 1);
    virtual ~ReadUrlLarge_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};
#endif