#include "writeurllarge_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "SP_Stream.h"

#include "url_filter.h"
#include "midfile.h"

#include "readurllarge_module.h"

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
extern SP_Stream *s_instance_stream;
void WriteUrlLarge_Module::svc()
{
    Request *data = gRequest;
    CRequest *c_data = NULL;
    SUrlFilter *filter = NULL;
    SPFFilter *spffilter = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        filter = reinterpret_cast<SUrlFilter *>(msg->data());
        if (filter != NULL)
        {
            int count = filter->file_size_;
            spffilter = filter->pf_;
            filter->write_tag(stdout);
            filter->release_buf();
            // SP_DEBUG("WriteUrlLarge_Module:send=%d,recv=%d\n", data->size_split_buf, data->recv_split_);
            s_instance_stream->put(msg);
            if (spffilter->is_ready(count))
            {
                gMCount.lock();
                ++data->recv_split_;
                gMCount.unlock();
                spffilter->release_buf();
                unique_lock<mutex> lock(gMutexUrlPfTask);
                gQUrlPfTask.push(spffilter);
                gCVUrlPfTask.notify_one();
            }
        }

        if (data->size_split_buf == data->recv_split_)
        {
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