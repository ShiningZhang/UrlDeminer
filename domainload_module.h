#pragma once
#ifndef DOMAINLOAD_MODULE_H_
#define DOMAINLOAD_MODULE_H_

#include "SP_Module.h"

#include <mutex>

class DomainLoad_Module : public SP_Module
{
public:
    static DomainLoad_Module *instance()
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

class DomainLoad_Module_v1 : public SP_Module
{
public:
    static DomainLoad_Module_v1 *instance()
    {
        return SP_Singleton<DomainLoad_Module_v1>::instance();
    }
    DomainLoad_Module_v1(int threads = 1);
    virtual ~DomainLoad_Module_v1();
    virtual int open();
    virtual void svc();
    virtual int init();

private:
    int threads_num_;
    std::mutex lock_;
};

class DomainLoad_Module_case2 : public SP_Module
{
public:
    static DomainLoad_Module_case2 *instance()
    {
        return SP_Singleton<DomainLoad_Module_case2>::instance();
    }
    DomainLoad_Module_case2(int threads = 1);
    virtual ~DomainLoad_Module_case2();
    virtual int open();
    virtual void svc();
    virtual int init();

private:
    int threads_num_;
    std::mutex lock_;
};
#endif