#ifndef MIDLARGE_MODULE_H_
#define MIDLARGE_MODULE_H_
#pragma once

#include "SP_Module.h"

#include <mutex>

class MidLarge_Module : public SP_Module
{
public:
    static MidLarge_Module *instance()
    {
        return SP_Singleton<MidLarge_Module>::instance();
    }
    MidLarge_Module(int threads = 1);
    virtual ~MidLarge_Module();
    virtual int open();
    virtual void svc();
    virtual int init();

private:
    int threads_num_;
    std::mutex lock_;
};
#endif