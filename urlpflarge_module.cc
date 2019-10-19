#include "urlpflarge_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "url_filter.h"
#include "midfile.h"

UrlPFLarge_Module::UrlPFLarge_Module(int threads)
    : threads_num_(threads)
{
}

UrlPFLarge_Module::~UrlPFLarge_Module()
{
}

int UrlPFLarge_Module::open()
{
    activate(threads_num_);
    return 0;
}

void UrlPFLarge_Module::svc()
{
    Request *data = gRequest;
    CRequest *c_data = NULL;
    UrlPFFilter *filter = NULL;
    while (true)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        {

            unique_lock<mutex> lock(gMutexUrlPfTask);
            if (gEnd)
            {
                return;
            }
            while (gQUrlPfTask.empty())
            {
                gCVUrlPfTask.wait(lock);
                if (gEnd)
                {
                    return;
                }
            }
            filter = gQUrlPfTask.front();
            gQUrlPfTask.pop();
        }
        filter->load_pf();
        filter->load_url();
        filter->pre_pf();
        filter->pre_url();
        filter->filter();

        {
            unique_lock<mutex> lock1(gMutexWriteUrlTask);
            gQWriteUrlTask.push(filter);
            gCVWriteUrlTask.notify_one();
        }

        gettimeofday(&t2, 0);
        SP_DEBUG("UrlPFLarge_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
    }
}

int UrlPFLarge_Module::init()
{
    return 0;
}