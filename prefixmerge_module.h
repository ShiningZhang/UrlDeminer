#pragma once
#ifndef PREFIXMERGE_MODULE_H_
#define PREFIXMERGE_MODULE_H_

#include "SP_Module.h"

#include <mutex>

class PrefixMerge_Module : public SP_Module
{
public:
    static PrefixMerge_Module *instance()
    {
        return SP_Singleton<PrefixMerge_Module>::instance();
    }
    PrefixMerge_Module(int threads = 1);
    virtual ~PrefixMerge_Module();
    virtual int open();
    virtual void svc();
    virtual int init();

private:
    int threads_num_;
    std::mutex lock_;
};

class PrefixMerge_Module_case2 : public SP_Module
{
public:
    static PrefixMerge_Module_case2 *instance()
    {
        return SP_Singleton<PrefixMerge_Module_case2>::instance();
    }
    PrefixMerge_Module_case2(int threads = 1);
    virtual ~PrefixMerge_Module_case2();
    virtual int open();
    virtual void svc();
    virtual int init();

private:
    int threads_num_;
    std::mutex lock_;
};

#endif