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

static int gidx_dp[2] = {0};

int DomainMerge_Module::open()
{
    memset(gidx_dp, 0, 2 * sizeof(int));
    activate(threads_num_);
    return 0;
}

void DomainMerge_Module::svc()
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
        SP_DES(msg);
        int idx = c_data->idx_;
        DomainFilter *filter = c_data->domain_filter_;
        SP_DES(c_data);
        int idx1 = 0;
        int idx2 = 0;
        {
            unique_lock<mutex> lock(lock_);
            idx1 = gidx_dp[0];
            idx2 = gidx_dp[1];
            ++gidx_dp[1];
            if (gidx_dp[1] == DOMAIN_CHAR_COUNT + 1)
            {
                ++gidx_dp[0];
                gidx_dp[1] = 0;
            }
        }
        while (idx1 < 3)
        {
            ((DomainFilterMerge *)filter)->merge(data->domain_filter_list_, idx1, idx2);
            {
                unique_lock<mutex> lock(lock_);
                idx1 = gidx_dp[0];
                idx2 = gidx_dp[1];
                ++gidx_dp[1];
                if (gidx_dp[1] == DOMAIN_CHAR_COUNT + 1)
                {
                    ++gidx_dp[0];
                    gidx_dp[1] = 0;
                }
            }
        }

        lock_.lock();
        data->recv_split_++;
        recv_idx = data->recv_split_;
        lock_.unlock();
        if (recv_idx == data->size_split_buf)
        {
            data->domain_filter_ = filter;
            ((DomainFilterMerge *)filter)->cpy_filter_list(data->domain_filter_list_);
            data->reset_para();

            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)data));
            put_next(msg);
        }

        gettimeofday(&t2, 0);
        SP_DEBUG("DomainMerge_Module=%ldms[%d].\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000, idx);
        //SP_LOGI("DomainMerge_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int DomainMerge_Module::init()
{
    return 0;
}

DomainMerge_Module_v1::DomainMerge_Module_v1(int threads)
    : threads_num_(threads)
{
}

DomainMerge_Module_v1::~DomainMerge_Module_v1()
{
}

int DomainMerge_Module_v1::open()
{
    memset(gidx_dp, 0, 2 * sizeof(int));
    activate(threads_num_);
    return 0;
}

void DomainMerge_Module_v1::svc()
{
    Request *data = gRequest;
    int recv_idx = 0;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        SP_DES(msg);
        DomainFilter *filter = data->domain_filter_;
        int idx1 = 0;
        int idx2 = 0;
        {
            lock_.lock();
            idx1 = gidx_dp[0];
            idx2 = gidx_dp[1];
            ++gidx_dp[1];
            if (gidx_dp[1] == DOMAIN_CHAR_COUNT + 1)
            {
                ++gidx_dp[0];
                gidx_dp[1] = 0;
            }
            lock_.unlock();
        }
        while (idx1 < 3)
        {
            ((DomainFilterMerge *)filter)->merge(data->domain_filter_list_, idx1, idx2);
            {
                lock_.lock();
                idx1 = gidx_dp[0];
                idx2 = gidx_dp[1];
                ++gidx_dp[1];
                if (gidx_dp[1] == DOMAIN_CHAR_COUNT + 1)
                {
                    ++gidx_dp[0];
                    gidx_dp[1] = 0;
                }
                lock_.unlock();
            }
        }

        lock_.lock();
        data->recv_split_++;
        recv_idx = data->recv_split_;
        lock_.unlock();
        if (recv_idx == data->size_split_buf)
        {
            data->domain_filter_ = filter;
            ((DomainFilterMerge *)filter)->cpy_filter_list(data->domain_filter_list_);
            data->reset_para();

            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)data));
            put_next(msg);
        }

        gettimeofday(&t2, 0);
        SP_DEBUG("DomainMerge_Module=%ldms[%d].\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000, recv_idx);
        //SP_LOGI("DomainMerge_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int DomainMerge_Module_v1::init()
{
    return 0;
}

DomainMerge_Module_case2::DomainMerge_Module_case2(int threads)
    : threads_num_(threads)
{
}

DomainMerge_Module_case2::~DomainMerge_Module_case2()
{
}

int DomainMerge_Module_case2::open()
{
    memset(gidx_dp, 0, 2 * sizeof(int));
    activate(threads_num_);
    return 0;
}

void DomainMerge_Module_case2::svc()
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
        SP_DES(msg);
        int idx = c_data->idx_;
        DomainFilter *filter = c_data->domain_filter_;
        SP_DES(c_data);
        int idx1 = 0;
        int idx2 = 0;
        {
            unique_lock<mutex> lock(lock_);
            idx1 = gidx_dp[0];
            idx2 = gidx_dp[1];
            ++gidx_dp[1];
            if (gidx_dp[1] == DOMAIN_CHAR_COUNT + 1)
            {
                ++gidx_dp[0];
                gidx_dp[1] = 0;
            }
        }
        while (idx1 < 3)
        {
            if (idx1 < 1 && idx2 < DOMAIN_CHAR_COUNT && !dp_need[idx2])
            {
                {
                    unique_lock<mutex> lock(lock_);
                    idx1 = gidx_dp[0];
                    idx2 = gidx_dp[1];
                    ++gidx_dp[1];
                    if (gidx_dp[1] == DOMAIN_CHAR_COUNT + 1)
                    {
                        ++gidx_dp[0];
                        gidx_dp[1] = 0;
                    }
                }
                continue;
            }
            if (idx1 > 0 && idx2 < DOMAIN_CHAR_COUNT && !dp_sp_need[idx1 - 1][idx2])
            {
                {
                    unique_lock<mutex> lock(lock_);
                    idx1 = gidx_dp[0];
                    idx2 = gidx_dp[1];
                    ++gidx_dp[1];
                    if (gidx_dp[1] == DOMAIN_CHAR_COUNT + 1)
                    {
                        ++gidx_dp[0];
                        gidx_dp[1] = 0;
                    }
                }
                continue;
            }
            ((DomainFilterMerge *)filter)->merge(data->domain_filter_list_, idx1, idx2);
            {
                unique_lock<mutex> lock(lock_);
                idx1 = gidx_dp[0];
                idx2 = gidx_dp[1];
                ++gidx_dp[1];
                if (gidx_dp[1] == DOMAIN_CHAR_COUNT + 1)
                {
                    ++gidx_dp[0];
                    gidx_dp[1] = 0;
                }
            }
        }

        lock_.lock();
        data->recv_split_++;
        recv_idx = data->recv_split_;
        lock_.unlock();
        if (recv_idx == data->size_split_buf)
        {
            data->domain_filter_ = filter;
            ((DomainFilterMerge *)filter)->cpy_filter_list(data->domain_filter_list_);
            data->reset_para();

            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)data));
            put_next(msg);
        }

        gettimeofday(&t2, 0);
        SP_DEBUG("DomainMerge_Module=%ldms[%d].\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000, idx);
        //SP_LOGI("DomainMerge_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int DomainMerge_Module_case2::init()
{
    return 0;
}