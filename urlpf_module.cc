#include "urlpf_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "url_filter.h"

UrlPF_Module::UrlPF_Module(int threads)
    : threads_num_(threads)
{
}

UrlPF_Module::~UrlPF_Module()
{
}

int UrlPF_Module::open()
{
    activate(threads_num_);
    return 0;
}

void UrlPF_Module::svc()
{
    CRequest *c_data = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        c_data = reinterpret_cast<CRequest *>(msg->data());
        {
            UrlFilter *filter = c_data->url_filter_;
            if (filter != NULL)
            {
                filter->prefixfilter_ = c_data->request_->prefix_filter_;
                filter->prepare_prefix();
                filter->filter_prefix();
            }
        }

        put_next(msg);

        gettimeofday(&t2, 0);
        SP_DEBUG("UrlPF_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("UrlPF_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int UrlPF_Module::init()
{
    return 0;
}