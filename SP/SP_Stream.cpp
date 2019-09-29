#include "SP_Stream.h"
#include "Global_Macros.h"

SP_Stream::SP_Stream()
    :head_(0),
    tail_(0)
{
    SP_TRACE("SP_Stream: SP_Stream()\n");
    this->open();
}

SP_Stream::~SP_Stream()
{
    SP_TRACE("SP_Stream: ~SP_Stream()\n");
}

void SP_Stream::push(SP_Module *module)
{
    SP_TRACE("SP_Stream::push\n");
    if (this->head_)
    {
        module->next(this->head_->next());
        this->head_->next(module);
    }
}

int SP_Stream::pop()
{
    SP_TRACE("SP_Stream::pop\n");
    int result = 0;
    if (this->head_ == 0)
    {
        return result;
    }
    SP_Module *top = 0;
    top = this->head_->next();
    this->head_->next(top->next());
    result = top->close();
    top->next(0);
    SP_DES(top);
    if (this->head_ == 0)
    {
        this->tail_ = 0;
    }
    return result;
}

int SP_Stream::push_module(SP_Module *module)
{
    SP_TRACE("SP_Stream::push_module\n");
    int result = 0;
    if (this->head_)
    {
        module->next(this->head_->next());
        this->head_->next(module);
        result |= module->open();
    }
    else
        result = -1;
    return result;
}

int SP_Stream::close()
{
    SP_TRACE("SP_Stream::close\n");
    int result = 0;
    while (this->head_->next() != this->tail_)
    {
        result |= this->pop();
    }
    result |= this->head_->close();
    this->head_->next(0);
    SP_DES(this->head_);
    result |= this->tail_->close();
    SP_DES(this->tail_);
    return result;
}

void SP_Stream::open()
{
    SP_TRACE("SP_Stream::open\n");

    if (this->tail_ != 0)
    {
        this->tail_->close();
        this->tail_->next(0);
        SP_DES(this->tail_);
    }
    SP_NEW(this->tail_, SP_Module);

    if (this->head_ != 0)
    {
        this->head_->close();
        this->head_->next(0);
        SP_DES(this->head_);
    }
    SP_NEW(this->head_, SP_Module);
    this->head_->next(this->tail_);
}

int SP_Stream::put(SP_Message_Block_Base *msg)
{
    SP_TRACE("SP_Stream::put\n");

    if (this->head_ == 0)
    {
        return -1;
    }
    return this->head_->put_next(msg);
}

int SP_Stream::get(SP_Message_Block_Base *&msg)
{
    SP_TRACE("SP_Stream::get\n");

    if (this->tail_ == 0)
    {
        return -1;
    }
    return this->tail_->get(msg);
}