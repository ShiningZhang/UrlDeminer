#include "prefixmerge_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "prefix_filter.h"

PrefixMerge_Module::PrefixMerge_Module(int threads)
    : threads_num_(threads)
{
}

PrefixMerge_Module::~PrefixMerge_Module()
{
}

int PrefixMerge_Module::open()
{
    activate(threads_num_);
    return 0;
}

void PrefixMerge_Module::svc()
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
        int idx = c_data->idx_;

        PrefixFilterMerge *filter = (PrefixFilterMerge *)data->prefix_filter_;
        if (filter != NULL)
        {
            for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
            {
                filter->merge(data->prefix_filter_list_, c_data->idx_, i);
            }
        }
        SP_DES(c_data);

        lock_.lock();
        data->recv_split_++;
        lock_.unlock();
        if (data->recv_split_ == data->size_split_buf)
        {
            filter->cpy_filter_list(data->prefix_filter_list_);
            data->reset_para();
            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)data));
            put_next(msg);
        }

        gettimeofday(&t2, 0);
        SP_DEBUG("PrefixMerge_Module=%ldms.idx=%d\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000, idx);
        //SP_LOGI("PrefixMerge_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int PrefixMerge_Module::init()
{
    return 0;
}

PrefixMerge_Module_case2::PrefixMerge_Module_case2(int threads)
    : threads_num_(threads)
{
}

PrefixMerge_Module_case2::~PrefixMerge_Module_case2()
{
}

int PrefixMerge_Module_case2::open()
{
    activate(threads_num_);
    return 0;
}

void PrefixMerge_Module_case2::svc()
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
        int idx = c_data->idx_;

        PrefixFilterMerge *filter = (PrefixFilterMerge *)data->prefix_filter_;
        if (filter != NULL)
        {
            for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
            {
                if (pf_need[idx, i])
                    filter->merge(data->prefix_filter_list_, c_data->idx_, i);
            }
        }
        SP_DES(c_data);

        lock_.lock();
        data->recv_split_++;
        lock_.unlock();
        if (data->recv_split_ == data->size_split_buf)
        {
            filter->cpy_filter_list(data->prefix_filter_list_);
            data->reset_para();
            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)data));
            put_next(msg);
        }

        gettimeofday(&t2, 0);
        SP_DEBUG("PrefixMerge_Module=%ldms.idx=%d\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000, idx);
        //SP_LOGI("PrefixMerge_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int PrefixMerge_Module_case2::init()
{
    return 0;
}