#pragma once
#ifndef UrlPF_Module_H_
#define UrlPF_Module_H_

#include "SP_Module.h"

#include <mutex>

class UrlPF_Module : public SP_Module
{
public:
    static UrlPF_Module *instance()
    {
        return SP_Singleton<UrlPF_Module>::instance();
    }
    UrlPF_Module(int threads = 1);
    virtual ~UrlPF_Module();
    virtual int open();
    virtual void svc();
    virtual int init();

private:
    int threads_num_;
    std::mutex lock_;
};

#endif