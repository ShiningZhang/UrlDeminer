#include "readurl2_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "url_filter.h"
#include "midfile.h"

ReadUrl2_Module::ReadUrl2_Module(int threads)
    : threads_num_(threads)
{
}

ReadUrl2_Module::~ReadUrl2_Module()
{
}

int ReadUrl2_Module::open()
{
    activate(threads_num_);
    return 0;
}

void ReadUrl2_Module::svc()
{
    Request *data = NULL;
    CRequest *c_data = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        UrlFilter *filter = NULL;
        data = reinterpret_cast<Request *>(msg->data());
        SP_DES(msg);
        MidFile *mid_file = data->mid_file_;
        data->size_split_buf = gQueueCache.size() + mid_file->file_list_.size();
        int split = 0;
        while (!gQueueCache.empty())
        {
            filter = gQueueCache.front();
            gQueueCache.pop();
            SP_NEW(c_data, CRequest(data));
            c_data->url_filter_ = filter;
            ++split;
            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)c_data));
            put_next(msg);
        }
        if (mid_file->file_list_.size() > 0)
        {
            unique_lock<mutex> lock(gMutex);
            while (gQueue.empty())
            {
                gCV.wait(lock);
            }
            filter = gQueue.front();
            gQueue.pop();
        }
        for (uint i = 0; i < mid_file->file_list_.size(); ++i)
        {
            FileElement *e = mid_file->file_list_[i];
            uint offset = 0;
            for (uint j = 0; j < DOMAIN_CHAR_COUNT; ++j)
            {
                uint size = readcontent_unlocked1(e->fp_[i], filter->p_ + offset, e->size_[i]);
                filter->load2(filter->p_ + offset, size, j);
                offset += size;
            }
            filter->size_ = offset;
            SP_NEW(c_data, CRequest(data));
            c_data->url_filter_ = filter;
            ++split;
            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)c_data));
            put_next(msg);
            {
                unique_lock<mutex> lock(gMutex);
                while (gQueue.empty())
                {
                    gCV.wait(lock);
                }
                filter = gQueue.front();
                gQueue.pop();
            }
        }

        data->is_read_end_ = true;
        for (int i = split; i < data->size_split_buf; ++i)
        {
            SP_NEW(c_data, CRequest(data));
            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)c_data));
            put_next(msg);
        }
        SP_DEBUG("ReadUrl2_Module:size_split_buf=%d,end\n", data->size_split_buf);
        gettimeofday(&t2, 0);
        SP_DEBUG("ReadUrl2_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("ReadUrl2_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int ReadUrl2_Module::init()
{
    return 0;
}