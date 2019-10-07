#include "urlload_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "url_filter.h"

UrlLoad_Module::UrlLoad_Module(int threads)
    : threads_num_(threads)
{
}

UrlLoad_Module::~UrlLoad_Module()
{
}

int UrlLoad_Module::open()
{
    activate(threads_num_);
    return 0;
}

void UrlLoad_Module::svc()
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
        

        UrlFilter *filter = UrlFilter::load(c_data->buffer_, c_data->size_);
        lock_.lock();
        data->url_filter_list_.push_back(filter);
        lock_.unlock();
        c_data->url_filter_list_.push_back(filter);
        filter->set_dp_list(data->domain_filter_list_);
        filter->filter_domainport();
        filter->set_pf_list(data->prefix_filter_list_);
        filter->filter_prefix();
        
        put_next(msg);

        gettimeofday(&t2, 0);
        SP_DEBUG("UrlLoad_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("UrlLoad_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int UrlLoad_Module::init()
{
    return 0;
}