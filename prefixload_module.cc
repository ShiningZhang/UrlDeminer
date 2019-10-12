#include "prefixload_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "prefix_filter.h"

PrefixLoad_Module::PrefixLoad_Module(int threads)
    : threads_num_(threads)
{
}

PrefixLoad_Module::~PrefixLoad_Module()
{
}

int PrefixLoad_Module::open()
{
    activate(threads_num_);
    return 0;
}

void PrefixLoad_Module::svc()
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

        PrefixFilter *filter = PrefixFilter::load(c_data->buffer_, c_data->size_);
        if (filter != NULL)
            data->prefix_filter_list_.push_back(filter);
        SP_DES(c_data);

        lock_.lock();
        data->recv_split_++;
        lock_.unlock();
        if (data->recv_split_ == data->size_split_buf && data->is_read_end_)
        {
            filter = PrefixFilter::merge(data->prefix_filter_list_);
            data->prefix_filter_list_.push_back(filter);
            data->reset_para();
            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)data));
            put_next(msg);
        }

        gettimeofday(&t2, 0);
        SP_DEBUG("PrefixLoad_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("PrefixLoad_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int PrefixLoad_Module::init()
{
    return 0;
}