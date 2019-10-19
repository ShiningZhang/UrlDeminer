#pragma once
#ifndef PREFIXWRITE_MODULE_H_
#define PREFIXWRITE_MODULE_H_

#include "SP_Module.h"

#include <mutex>

class PrefixWrite_Module : public SP_Module
{
public:
    static PrefixWrite_Module *instance()
    {
        return SP_Singleton<PrefixWrite_Module>::instance();
    }
    PrefixWrite_Module(int threads = 1);
    virtual ~PrefixWrite_Module();
    virtual int open();
    virtual void svc();
    virtual int init();

private:
    int threads_num_;
    std::mutex lock_;
};

#endif