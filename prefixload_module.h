#pragma once
#ifndef PREFIX_MODULE_H_
#define PREFIX_MODULE_H_

#include "SP_Module.h"

#include <mutex>

class PrefixLoad_Module : public SP_Module
{
public:
    static PrefixLoad_Module * instance()
    {
        return SP_Singleton<PrefixLoad_Module>::instance();
    }
    PrefixLoad_Module(int threads = 1);
    virtual ~PrefixLoad_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};

#endif