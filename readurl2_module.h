#ifndef READURL2_MODULE_H_
#define READURL2_MODULE_H_
#pragma once

#include "SP_Module.h"

#include <mutex>

class ReadUrl2_Module : public SP_Module
{
public:
    static ReadUrl2_Module * instance()
    {
        return SP_Singleton<ReadUrl2_Module>::instance();
    }
    ReadUrl2_Module(int threads = 1);
    virtual ~ReadUrl2_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};
#endif