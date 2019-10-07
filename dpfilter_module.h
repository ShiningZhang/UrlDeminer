#pragma once
#ifndef DPFILTER_MODULE_H_
#define DPFILTER_MODULE_H_

#include "SP_Module.h"

#include <mutex>

class DPFilter_Module : public SP_Module
{
public:
    static DPFilter_Module * instance()
    {
        return SP_Singleton<DPFilter_Module>::instance();
    }
    DPFilter_Module(int threads = 1);
    virtual ~DPFilter_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};

#endif