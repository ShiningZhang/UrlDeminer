#pragma once
#ifndef UrlDP_Module_H_
#define UrlDP_Module_H_

#include "SP_Module.h"

#include <mutex>

class UrlDP_Module : public SP_Module
{
public:
    static UrlDP_Module * instance()
    {
        return SP_Singleton<UrlDP_Module>::instance();
    }
    UrlDP_Module(int threads = 1);
    virtual ~UrlDP_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};

#endif