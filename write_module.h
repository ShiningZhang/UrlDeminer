#pragma once
#ifndef WRITE_MODULE_H_
#define WRITE_MODULE_H_

#include "SP_Module.h"

#include <mutex>

class Write_Module : public SP_Module
{
public:
    static Write_Module * instance()
    {
        return SP_Singleton<Write_Module>::instance();
    }
    Write_Module(int threads = 1);
    virtual ~Write_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};

#endif