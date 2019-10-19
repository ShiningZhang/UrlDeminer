#include "writeurllarge_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "url_filter.h"
#include "midfile.h"

WriteUrlLarge_Module::WriteUrlLarge_Module(int threads)
    : threads_num_(threads)
{
}

WriteUrlLarge_Module::~WriteUrlLarge_Module()
{
}

int WriteUrlLarge_Module::open()
{
    activate(threads_num_);
    return 0;
}

void WriteUrlLarge_Module::svc()
{
    Request *data = gRequest;
    CRequest *c_data = NULL;
    UrlPFFilter *filter = NULL;
    while (true)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        {

            unique_lock<mutex> lock(gMutexWriteUrlTask);
            if (gEnd)
            {
                return;
            }
            while (gQWriteUrlTask.empty())
            {
                gCVWriteUrlTask.wait(lock);
                if (gEnd)
                {
                    return;
                }
            }
            filter = gQWriteUrlTask.front();
            gQWriteUrlTask.pop();
        }
        filter->write_tag(stdout);
        filter->release_buf();
        gMCount.lock();
        ++data->recv_split_;
        gMCount.unlock();

        {
            unique_lock<mutex> lock1(gMutex);
            gQueueFilter.push(filter);
            gCV.notify_one();
        }
        if (data->size_split_buf == data->recv_split_)
        {
            SP_Message_Block_Base *msg;
            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)data));
            put_next(msg);
        }

        gettimeofday(&t2, 0);
        SP_DEBUG("WriteUrlLarge_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
    }
}

int WriteUrlLarge_Module::init()
{
    return 0;
}