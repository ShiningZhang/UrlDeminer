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
    Request *data = NULL;
    CRequest *c_data = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        c_data = reinterpret_cast<CRequest *>(msg->data());
        data = c_data->request_;

        UrlFilter *filter = UrlFilter::load(c_data->buffer_, c_data->size_);
        c_data->url_filter_ = (filter);

        SP_DEBUG("UrlLoad_Module:filter=%p\n", filter);
        if (filter != NULL)
        {
            lock_.lock();
            data->url_filter_list_.push_back(filter);
            lock_.unlock();
            SP_DEBUG("UrlLoad_Module:1\n");
            c_data->url_filter_ = (filter);
            SP_DEBUG("UrlLoad_Module:2\n");
            filter->set_dp_list(data->domain_filter_list_);
            filter->domainfilter_ = data->domain_filter_;
            SP_DEBUG("UrlLoad_Module:3\n");
            filter->filter_domainport();
            SP_DEBUG("UrlLoad_Module:4\n");
            filter->set_pf_list(data->prefix_filter_list_);
            filter->prefixfilter_ = data->prefix_filter_;
            SP_DEBUG("UrlLoad_Module:5\n");
            filter->prepare_prefix();
            SP_DEBUG("UrlLoad_Module:6\n");
            filter->filter_prefix();
            SP_DEBUG("UrlLoad_Module:7\n");
        }

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

UrlLoad_Module_case2::UrlLoad_Module_case2(int threads)
    : threads_num_(threads)
{
}

UrlLoad_Module_case2::~UrlLoad_Module_case2()
{
}

int UrlLoad_Module_case2::open()
{
    activate(threads_num_);
    return 0;
}

void UrlLoad_Module_case2::svc()
{
    Request *data = NULL;
    CRequest *c_data = NULL;
    int recv_idx = 0;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        c_data = reinterpret_cast<CRequest *>(msg->data());
        data = c_data->request_;

        UrlFilter *filter = UrlFilter::load(c_data->buffer_, c_data->size_);
        c_data->url_filter_ = (filter);

        SP_DEBUG("UrlLoad_Module_case2:filter=%p\n", filter);
        if (filter != NULL)
        {
            lock_.lock();
            data->url_filter_list_.push_back(filter);
            lock_.unlock();
        }
        lock_.lock();
        data->recv_split_++;
        recv_idx = data->recv_split_;
        lock_.unlock();
        if (recv_idx == data->size_split_buf)
        {
            data->reset_para();

            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)data));
            put_next(msg);
        }

        gettimeofday(&t2, 0);
        SP_DEBUG("UrlLoad_Module_case2=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("UrlLoad_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int UrlLoad_Module_case2::init()
{
    return 0;
}