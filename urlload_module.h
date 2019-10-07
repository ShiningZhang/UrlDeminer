#pragma once
#ifndef URLLOAD_MODULE_H_
#define URLLOAD_MODULE_H_

#include "SP_Module.h"

#include <mutex>

class UrlLoad_Module : public SP_Module
{
public:
    static UrlLoad_Module * instance()
    {
        return SP_Singleton<UrlLoad_Module>::instance();
    }
    UrlLoad_Module(int threads = 1);
    virtual ~UrlLoad_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};

#endif