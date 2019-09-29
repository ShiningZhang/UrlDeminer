#include "SP_Message_Queue.h"
#include "SP_Message_Block_Base.h"
#include "Global_Macros.h"

SP_Message_Queue::SP_Message_Queue(int hwm)
    :high_water_mark_(hwm),
    cur_count_(0),
    head_(0),
    tail_(0),
    state_(DEACTIVATED)
{
    SP_TRACE("SP_Message_Queue::SP_Message_Queue\n");
    this->open();
}

SP_Message_Queue::~SP_Message_Queue()
{
    SP_TRACE("SP_Message_Queue::~SP_Message_Queue\n");

    this->close();
}

int SP_Message_Queue::enqueue(SP_Message_Block_Base *msg)
{
    SP_TRACE("SP_Message_Queue::enqueue\n");

    int queue_count = 0;
    {
        std::unique_lock<std::mutex> lock(this->mutex_);
        if (this->state_ == DEACTIVATED)
        {
            return -1;
        }
        while (this->is_full_i())
        {
            not_full_cond_.wait(lock);
            if (this->state_ != ACTIVATED)
            {
                return -1;
            }
        }
        queue_count = this->enqueue_i(msg);
    }
    return queue_count;
}

int SP_Message_Queue::dequeue(SP_Message_Block_Base *&msg)
{
    SP_TRACE("SP_Message_Queue::dequeue\n");

    std::unique_lock<std::mutex> lock(this->mutex_);
    if (this->state_ == DEACTIVATED)
    {
        return -1;
    }
    while (this->is_empty_i())
    {
        not_empty_cond_.wait(lock);
        if (this->state_!=ACTIVATED)
        {
            return -1;
        }
    }
    return this->dequeue_i(msg);
}

bool SP_Message_Queue::is_full()
{
    SP_TRACE("SP_Message_Queue::is_full\n");

    std::lock_guard<std::mutex> lock(this->mutex_);
    return this->is_full_i();
}

bool SP_Message_Queue::is_empty()
{
    SP_TRACE("SP_Message_Queue::is_empty\n");

    std::lock_guard<std::mutex> lock(this->mutex_);
    return this->is_empty_i();
}

int SP_Message_Queue::flush()
{
    SP_TRACE("SP_Message_Queue::flush\n");

    std::lock_guard<std::mutex> lock(this->mutex_);
    return this->flush_i();
}

int SP_Message_Queue::close()
{
    SP_TRACE("SP_Message_Queue::close\n");

    std::lock_guard<std::mutex> lock(this->mutex_);
    if (this->state_ != DEACTIVATED)
    {
        this->not_empty_cond_.notify_all();
        this->not_full_cond_.notify_all();
        this->state_ = DEACTIVATED;
    }
    return this->flush_i();
}

void SP_Message_Queue::open()
{
    SP_TRACE("SP_Message_Queue::open\n");

    std::lock_guard<std::mutex> lock(this->mutex_);
    this->state_ = ACTIVATED;
}

int SP_Message_Queue::enqueue_i(SP_Message_Block_Base *msg)
{
    SP_TRACE("SP_Message_Queue::enqueue_i\n");

    if (msg == 0)
    {
        return -1;
    }

    ++this->cur_count_;
    if (this->tail_ == 0)
    {
        this->head_ = msg;
        this->tail_ = msg;
    }
    else
    {
        this->tail_->next(msg);
        this->tail_ = msg;
    }
    this->not_empty_cond_.notify_one();
    return this->cur_count_;
}

int SP_Message_Queue::dequeue_i(SP_Message_Block_Base *&msg)
{
    SP_TRACE("SP_Message_Queue::dequeue_i\n");

    if (this->head_ == 0)
    {
        return -1;
    }

    msg = this->head_;
    this->head_ = this->head_->next();

    if (this->head_ == 0)
    {
        this->tail_ = 0;
    }

    --this->cur_count_;

    msg->next(0);

    this->not_full_cond_.notify_one();

    return this->cur_count_;
}

bool SP_Message_Queue::is_full_i()
{
    SP_TRACE("SP_Message_Queue::is_full_i\n");

    return this->cur_count_ >= this->high_water_mark_;
}

bool SP_Message_Queue::is_empty_i()
{
    SP_TRACE("SP_Message_Queue::is_empty_i\n");

    return this->tail_ == 0;
}

int SP_Message_Queue::flush_i()
{
    SP_TRACE("SP_Message_Queue::flush_i\n");

    for (this->tail_ = 0; this->head_ != 0;)
    {
        --this->cur_count_;
        SP_Message_Block_Base *temp = this->head_;
        this->head_ = this->head_->next();
        SP_DES(temp);
    }
    return this->cur_count_ == 0;
}