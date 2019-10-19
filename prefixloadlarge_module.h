#pragma once
#ifndef PREFIXLOADLARGE_MODULE_H_
#define PREFIXLOADLARGE_MODULE_H_

#include "SP_Module.h"

#include <mutex>

class PrefixLoadLarge_Module : public SP_Module
{
public:
    static PrefixLoadLarge_Module * instance()
    {
        return SP_Singleton<PrefixLoadLarge_Module>::instance();
    }
    PrefixLoadLarge_Module(int threads = 1);
    virtual ~PrefixLoadLarge_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};

#endif