#include "readurl1_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "url_filter.h"

ReadUrl1_Module::ReadUrl1_Module(int threads)
    : threads_num_(threads)
{
}

ReadUrl1_Module::~ReadUrl1_Module()
{
}

int ReadUrl1_Module::open()
{
    activate(threads_num_);
    return 0;
}

void ReadUrl1_Module::svc()
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
        data = reinterpret_cast<Request *>(msg->data());
        SP_DES(msg);
        int end = 0;
        uint64_t length = data->length_;
        uint32_t size = 0;
        uint64_t begin = 0;
        UrlFilter *filter = NULL;
        size_t line_size = length < FILESPLITSIZE ? ceil((double)length / 8) : FILESPLITSIZE;
        SP_DEBUG("ReadUrl1_Module:length=%lld,line_size=%lld\n", length, line_size);
        while (begin < length)
        {
            if (begin + line_size > length)
                line_size = length - begin;
            {
                unique_lock<mutex> lock(gMutex);
                while (gQueue.empty())
                {
                    gCV.wait(lock);
                }
                filter = gQueue.front();
                gQueue.pop();
            }
            buf = filter->p_;

            size = readcontent_unlocked1(data->fp_in_, buf, line_size);
            begin += size;
            filter->size_ = size;

            SP_NEW(c_data, CRequest(data));
            c_data->buffer_ = buf;
            c_data->size_ = size;
            c_data->url_filter_ = (filter);
            c_data->idx_ = data->size_split_buf;

            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)c_data));
            ++data->size_split_buf;
            put_next(msg);
        }
        data->is_read_end_ = true;
        SP_DEBUG("ReadUrl1_Module:size_split_buf=%d,end\n", data->size_split_buf);
        gettimeofday(&t2, 0);
        SP_DEBUG("ReadUrl1_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("ReadUrl1_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int ReadUrl1_Module::init()
{
    return 0;
}