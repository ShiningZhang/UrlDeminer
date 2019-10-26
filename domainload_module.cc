#include "domainload_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "domain_filter.h"

DomainLoad_Module::DomainLoad_Module(int threads)
    : threads_num_(threads)
{
}

DomainLoad_Module::~DomainLoad_Module()
{
}

int DomainLoad_Module::open()
{
    activate(threads_num_);
    return 0;
}

void DomainLoad_Module::svc()
{
    Request *data = NULL;
    CRequest *c_data = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        c_data = reinterpret_cast<CRequest *>(msg->data());
        data = c_data->request_;
        SP_DES(msg);

        DomainFilter *filter = DomainFilter::load(c_data->buffer_, c_data->size_);
        if (filter != NULL)
        {
            lock_.lock();
            data->domain_filter_list_.push_back(filter);
            lock_.unlock();
        }
        SP_DES(c_data);

        lock_.lock();
        data->recv_split_++;
        lock_.unlock();
        if (data->recv_split_ == data->size_split_buf && data->is_read_end_)
        {
            data->reset_para();
            data->size_split_buf = 8;
            filter = new DomainFilterMerge();
            for (int i = 0; i < 8; ++i)
            {
                // for (int j = 0; j < 2; ++j)
                {
                    SP_NEW(c_data, CRequest(data));
                    c_data->domain_filter_ = filter;
                    c_data->idx_ = i;
                    // c_data->idx_list_[0] = j;
                    // c_data->idx_list_[1] = i;
                    SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)c_data));
                    put_next(msg);
                }
            }
        }

        gettimeofday(&t2, 0);
        SP_DEBUG("DomainLoad_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("DomainLoad_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int DomainLoad_Module::init()
{
    return 0;
}

DomainLoad_Module_v1::DomainLoad_Module_v1(int threads)
    : threads_num_(threads)
{
}

DomainLoad_Module_v1::~DomainLoad_Module_v1()
{
}

int DomainLoad_Module_v1::open()
{
    activate(threads_num_);
    return 0;
}

void DomainLoad_Module_v1::svc()
{
    Request *data = gRequest;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        char *buf = reinterpret_cast<char *>(msg->data());
        uint size = msg->msg_type();
        SP_DES(msg);

        DomainFilter *filter = DomainFilter::load(buf, size);
        if (filter != NULL)
        {
            // filter->compress();
            lock_.lock();
            data->domain_filter_list_.push_back(filter);
            lock_.unlock();
        }

        lock_.lock();
        data->recv_split_++;
        lock_.unlock();
        if (data->recv_split_ == data->size_split_buf && data->is_read_end_)
        {
            data->reset_para();
            data->size_split_buf = 8;
            filter = new DomainFilterMerge();
            data->domain_filter_ = filter;
            for (int i = 0; i < 8; ++i)
            {
                // for (int j = 0; j < 2; ++j)
                {
                    SP_NEW(msg, SP_Message_Block_Base());
                    put_next(msg);
                }
            }
        }

        gettimeofday(&t2, 0);
        SP_DEBUG("DomainLoad_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("DomainLoad_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int DomainLoad_Module_v1::init()
{
    return 0;
}

DomainLoad_Module_case2::DomainLoad_Module_case2(int threads)
    : threads_num_(threads)
{
}

DomainLoad_Module_case2::~DomainLoad_Module_case2()
{
}

int DomainLoad_Module_case2::open()
{
    activate(threads_num_);
    return 0;
}

void DomainLoad_Module_case2::svc()
{
    Request *data = NULL;
    CRequest *c_data = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        c_data = reinterpret_cast<CRequest *>(msg->data());
        data = c_data->request_;
        SP_DES(msg);

        DomainFilter *filter = DomainFilter::load_case2(c_data->buffer_, c_data->size_);
        if (filter != NULL)
        {
            lock_.lock();
            data->domain_filter_list_.push_back(filter);
            lock_.unlock();
        }
        SP_DES(c_data);

        lock_.lock();
        data->recv_split_++;
        lock_.unlock();
        if (data->recv_split_ == data->size_split_buf && data->is_read_end_)
        {
            data->reset_para();
            data->size_split_buf = 8;
            filter = new DomainFilterMerge();
            for (int i = 0; i < 8; ++i)
            {
                // for (int j = 0; j < 2; ++j)
                {
                    SP_NEW(c_data, CRequest(data));
                    c_data->domain_filter_ = filter;
                    c_data->idx_ = i;
                    // c_data->idx_list_[0] = j;
                    // c_data->idx_list_[1] = i;
                    SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)c_data));
                    put_next(msg);
                }
            }
        }

        gettimeofday(&t2, 0);
        SP_DEBUG("DomainLoad_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("DomainLoad_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int DomainLoad_Module_case2::init()
{
    return 0;
}