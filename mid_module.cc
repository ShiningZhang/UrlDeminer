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
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    char *buf;
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

        if (data->is_read_end_)
        {
            gQueueCache.push(filter);
            if (data->recv_split_ == data->size_split_buf)
            {
                mid_file->sort_file_list();
                SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)data));
                put_next(msg);
            }
        }
        else
        {
            mid_file->write_mid(filter->list_, filter->list_count_, mid_file->wt_size_, mid_file->buf_, filter->size_, c_data->idx_);
            {
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