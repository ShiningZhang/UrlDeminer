#include "mid_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "url_filter.h"
#include "midfile.h"

Mid_Module::Mid_Module(int threads)
    : threads_num_(threads)
{
}

Mid_Module::~Mid_Module()
{
}

int Mid_Module::open()
{
    activate(threads_num_);
    return 0;
}

void Mid_Module::svc()
{
    Request *data = NULL;
    CRequest *c_data = NULL;
    MidFile *mid_file = NULL;
    UrlFilter *filter = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        c_data = reinterpret_cast<CRequest *>(msg->data());
        SP_DES(msg);
        data = c_data->request_;
        ++data->recv_split_;
        mid_file = data->mid_file_;
        filter = c_data->url_filter_;

        data->counter_.pass += filter->counters_.pass;
        data->counter_.hit += filter->counters_.hit;
        data->counter_.miss += filter->counters_.miss;
        data->counter_.passchecksum ^= filter->counters_.passchecksum;
        data->counter_.hitchecksum ^= filter->counters_.hitchecksum;
        filter->clear_counter();

        if (data->is_read_end_)
        {
            gQueueCache.push(filter);
            if (data->recv_split_ == data->size_split_buf)
            {
                mid_file->sort_file_list();
                data->reset_para();
                SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)data));
                put_next(msg);
            }
        }
        else
        {
#ifdef LARGE
            mid_file->write_mid(filter->list_, filter->list_count_, c_data->idx_);
#endif
            {
                filter->clear_para();
                unique_lock<mutex> lock(gMutex);
                gQueue.push(filter);
                gCV.notify_one();
            }
        }

        gettimeofday(&t2, 0);
        SP_DEBUG("Mid_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("Mid_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int Mid_Module::init()
{
    return 0;
}