#pragma once
#ifndef DOMAINMERGE_MODULE_H_
#define DOMAINMERGE_MODULE_H_

#include "SP_Module.h"

#include <mutex>

class DomainMerge_Module : public SP_Module
{
public:
    static DomainMerge_Module *instance()
    {
        return SP_Singleton<DomainMerge_Module>::instance();
    }
    DomainMerge_Module(int threads = 1);
    virtual ~DomainMerge_Module();
    virtual int open();
    virtual void svc();
    virtual int init();

private:
    int threads_num_;
    std::mutex lock_;
};

class DomainMerge_Module_v1 : public SP_Module
{
public:
    static DomainMerge_Module_v1 *instance()
    {
        return SP_Singleton<DomainMerge_Module_v1>::instance();
    }
    DomainMerge_Module_v1(int threads = 1);
    virtual ~DomainMerge_Module_v1();
    virtual int open();
    virtual void svc();
    virtual int init();

private:
    int threads_num_;
    std::mutex lock_;
};

class DomainMerge_Module_case2 : public SP_Module
{
public:
    static DomainMerge_Module_case2 *instance()
    {
        return SP_Singleton<DomainMerge_Module_case2>::instance();
    }
    DomainMerge_Module_case2(int threads = 1);
    virtual ~DomainMerge_Module_case2();
    virtual int open();
    virtual void svc();
    virtual int init();

private:
    int threads_num_;
    std::mutex lock_;
};

#endif