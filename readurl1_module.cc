#include "readurl1_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "url_filter.h"
#include "midlarge_module.h"

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
    char *buf;
    Request *data = NULL;
    SP_Message_Block_Base *msg = 0;
    // for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        get(msg);
        data = reinterpret_cast<Request *>(msg->data());
        SP_DES(msg);
        timeval t2, start;
        gettimeofday(&start, 0);
        uint64_t length = data->length_;
        uint64_t size = 0;
        uint64_t begin = 0;
        UrlFilterLarge *filter = NULL;
        uint64_t line_size = data->split_size_;
        data->size_split_buf = (int)ceil((double)length / line_size);
        int split = 0;
        SP_DEBUG("ReadUrl1_Module:length=%lld,line_size=%lld,size_split_buf=%d \n", length, line_size, data->size_split_buf);
        while (begin < length)
        {
            if (begin + line_size > length)
                line_size = length - begin;
            {
                get(msg);
                filter = reinterpret_cast<UrlFilterLarge *>(msg->data());
            }
            buf = filter->p_;

            size = readcontent_unlocked1(data->fp_in_, buf, line_size);
            begin += size;
            filter->size_ = size;
            filter->idx_ = split++;

            put_next(msg);
        }
        data->is_read_end_ = true;
        if (data->recv_split_ == data->size_split_buf)
        {
            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)data));
            MidLarge_Module::instance()->put_next(msg);
        }
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