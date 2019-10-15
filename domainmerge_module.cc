#include "domainmerge_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "domain_filter.h"

DomainMerge_Module::DomainMerge_Module(int threads)
    : threads_num_(threads)
{
}

DomainMerge_Module::~DomainMerge_Module()
{
}

int DomainMerge_Module::open()
{
    activate(threads_num_);
    return 0;
}

void DomainMerge_Module::svc()
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

        DomainFilter *filter = c_data->domain_filter_;
        filter->merge(data->domain_filter_list_, c_data->idx_);

        SP_DES(c_data);

        lock_.lock();
        data->recv_split_++;
        lock_.unlock();
        if (data->recv_split_ == data->size_split_buf)
        {
            data->domain_filter_ = filter;
            data->reset_para();

            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)data));
            put_next(msg);
        }

        gettimeofday(&t2, 0);
        SP_DEBUG("DomainMerge_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("DomainMerge_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int DomainMerge_Module::init()
{
    return 0;
}