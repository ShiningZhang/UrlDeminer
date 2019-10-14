#ifndef READURL1_MODULE_H_
#define READURL1_MODULE_H_
#pragma once

#include "SP_Module.h"

#include <mutex>

class ReadUrl1_Module : public SP_Module
{
public:
    static ReadUrl1_Module * instance()
    {
        return SP_Singleton<ReadUrl1_Module>::instance();
    }
    ReadUrl1_Module(int threads = 1);
    virtual ~ReadUrl1_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};
#endif