#include "urldp1_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "url_filter.h"

UrlDP1_Module::UrlDP1_Module(int threads)
    : threads_num_(threads)
{
}

UrlDP1_Module::~UrlDP1_Module()
{
}

int UrlDP1_Module::open()
{
    activate(threads_num_);
    return 0;
}

void UrlDP1_Module::svc()
{
    UrlFilterLarge *filter = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        filter = reinterpret_cast<UrlFilterLarge *>(msg->data());
        {
            if (filter != NULL && filter->size_ > 0)
            {
                filter->load1();
                ((UrlFilterLarge *)filter)->filter_domainport_large();
            }
        }

        put_next(msg);

        gettimeofday(&t2, 0);
        SP_DEBUG("UrlDP1_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("UrlDP1_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int UrlDP1_Module::init()
{
    return 0;
}