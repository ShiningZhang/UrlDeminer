#ifndef MID_MODULE_H_
#define MID_MODULE_H_
#pragma once

#include "SP_Module.h"

#include <mutex>

class Mid_Module : public SP_Module
{
public:
    static Mid_Module *instance()
    {
        return SP_Singleton<Mid_Module>::instance();
    }
    Mid_Module(int threads = 1);
    virtual ~Mid_Module();
    virtual int open();
    virtual void svc();
    virtual int init();

private:
    int threads_num_;
    std::mutex lock_;
};
#endif