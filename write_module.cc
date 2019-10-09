#include "write_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "url_filter.h"

Write_Module::Write_Module(int threads)
    : threads_num_(threads)
{
}

Write_Module::~Write_Module()
{
}

int Write_Module::open()
{
    activate(threads_num_);
    return 0;
}

void Write_Module::svc()
{
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    char *buf;
    Request *data = NULL;
    CRequest *c_data = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        c_data = reinterpret_cast<CRequest *>(msg->data());
        data = c_data->request_;
        SP_DES(msg);

        for (int i = 0; i < c_data->url_filter_list_.size(); ++i)
        {
            UrlFilter *filter = c_data->url_filter_list_[i];
            filter->write_tag(data->fp_out_);
            data->counter_.pass += filter->counters_.pass;
            data->counter_.hit += filter->counters_.hit;
            data->counter_.miss += filter->counters_.miss;
            data->counter_.passchecksum ^= filter->counters_.passchecksum;
            data->counter_.hitchecksum ^= filter->counters_.hitchecksum;
        }
        SP_DES(c_data);

        lock_.lock();
        data->recv_split_++;
        SP_DEBUG("Write_Module:recv_split_=%d,size_split_buf=%d\n", data->recv_split_, data->size_split_buf);
        lock_.unlock();
        if (data->recv_split_ == data->size_split_buf && data->is_read_end_)
        {
            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)data));
            put_next(msg);
        }

        gettimeofday(&t2, 0);
        SP_DEBUG("Write_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("Write_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int Write_Module::init()
{
    return 0;
}