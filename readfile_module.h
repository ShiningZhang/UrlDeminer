#ifndef READFILE_MODULE_H_
#define READFILE_MODULE_H_
#pragma once

#include "SP_Module.h"

#include <mutex>

class ReadFile_Module : public SP_Module
{
public:
    static ReadFile_Module * instance()
    {
        return SP_Singleton<ReadFile_Module>::instance();
    }
    ReadFile_Module(int threads = 1);
    virtual ~ReadFile_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};

class ReadFile_Module_v1 : public SP_Module
{
public:
    static ReadFile_Module_v1 * instance()
    {
        return SP_Singleton<ReadFile_Module_v1>::instance();
    }
    ReadFile_Module_v1(int threads = 1);
    virtual ~ReadFile_Module_v1();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};
#endif