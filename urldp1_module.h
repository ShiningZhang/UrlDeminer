#pragma once
#ifndef UrlDP1_Module_H_
#define UrlDP1_Module_H_

#include "SP_Module.h"

#include <mutex>

class UrlDP1_Module : public SP_Module
{
public:
    static UrlDP1_Module * instance()
    {
        return SP_Singleton<UrlDP1_Module>::instance();
    }
    UrlDP1_Module(int threads = 1);
    virtual ~UrlDP1_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};

#endif