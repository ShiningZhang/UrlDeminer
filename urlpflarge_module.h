#ifndef URLPFLARGE_MODULE_H_
#define URLPFLARGE_MODULE_H_
#pragma once

#include "SP_Module.h"

#include <mutex>

class UrlPFLarge_Module : public SP_Module
{
public:
    static UrlPFLarge_Module * instance()
    {
        return SP_Singleton<UrlPFLarge_Module>::instance();
    }
    UrlPFLarge_Module(int threads = 1);
    virtual ~UrlPFLarge_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};
#endif