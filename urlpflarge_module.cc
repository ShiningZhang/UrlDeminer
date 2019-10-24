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
    SUrlFilter *filter = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        filter = reinterpret_cast<SUrlFilter *>(msg->data());
        if (filter != NULL)
        {
            filter->load_url();
            filter->pre_url();
            filter->filter();
        }
        put_next(msg);

        gettimeofday(&t2, 0);
        SP_DEBUG("UrlPFLarge_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
    }
}

int UrlPFLarge_Module::init()
{
    return 0;
}