#ifndef SP_STREAM_H
#define SP_STREAM_H

#pragma once

#include "SP_Module.h"
#include "SP_Message_Block_Base.h"

class SP_Stream
{
public:
    SP_Stream();
    virtual ~SP_Stream();
    virtual void push(SP_Module *);
    virtual int pop();
    virtual int push_module(SP_Module *);
    virtual int close();
    virtual void open();
    virtual int put(SP_Message_Block_Base *);
    virtual int get(SP_Message_Block_Base *&);
private:
    SP_Module *head_;
    SP_Module *tail_;
};

#endif
