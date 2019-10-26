#include "midlarge_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "url_filter.h"
#include "midfile.h"

MidLarge_Module::MidLarge_Module(int threads)
    : threads_num_(threads)
{
}

MidLarge_Module::~MidLarge_Module()
{
}

int MidLarge_Module::open()
{
    activate(threads_num_);
    return 0;
}

void MidLarge_Module::svc()
{
    Request *data = gRequest;
    MidFile *mid_file = NULL;
    UrlFilterLarge *filter = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        ++data->recv_split_;
        mid_file = data->mid_file_;
        filter = reinterpret_cast<UrlFilterLarge *>(msg->data());

        if (filter != NULL)
        {
            data->counter_.pass += filter->counters_.pass;
            data->counter_.hit += filter->counters_.hit;
            data->counter_.miss += filter->counters_.miss;
            data->counter_.passchecksum ^= filter->counters_.passchecksum;
            data->counter_.hitchecksum ^= filter->counters_.hitchecksum;
            filter->clear_counter();
        }

        SP_DEBUG("MidLarge_Module:is_read_end_=%d,recv_split_=%d,size_split_buf=%d,filter=%p\n", data->is_read_end_, data->recv_split_, data->size_split_buf, filter);

        {
            if (filter != NULL)
            {
                if (filter->size_ > 0)
                {
                    filter->prepare_write();
                    mid_file->write_mid_large(filter, filter->list_write_count_, filter->idx_);
                }
                filter->clear_para_large();
                put_next(msg);
            }
            if (data->recv_split_ == data->size_split_buf && data->is_read_end_)
            {
                mid_file->sort_file_list();
                data->reset_para();
                SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)data));
                put_next(msg);
            }
        }
        gettimeofday(&t2, 0);
        SP_DEBUG("MidLarge_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("MidLarge_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int MidLarge_Module::init()
{
    return 0;
}