#pragma once
#ifndef PREFIXMERGELARGE_MODULE_H_
#define PREFIXMERGELARGE_MODULE_H_

#include "SP_Module.h"

#include <mutex>

class PrefixMergeLarge_Module : public SP_Module
{
public:
    static PrefixMergeLarge_Module *instance()
    {
        return SP_Singleton<PrefixMergeLarge_Module>::instance();
    }
    PrefixMergeLarge_Module(int threads = 1);
    virtual ~PrefixMergeLarge_Module();
    virtual int open();
    virtual void svc();
    virtual int init();

private:
    int threads_num_;
    std::mutex lock_;
};

#endif