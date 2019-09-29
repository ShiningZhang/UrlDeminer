#pragma once
#ifndef DOMAINLOAD_MODULE_H_
#define DOMAINLOAD_MODULE_H_

#include "SP_Module.h"

#include <mutex>

class DomainLoad_Module : public SP_Module
{
public:
    static DomainLoad_Module * instance()
    {
        return SP_Singleton<DomainLoad_Module>::instance();
    }
    DomainLoad_Module(int threads = 1);
    virtual ~DomainLoad_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};

#endif