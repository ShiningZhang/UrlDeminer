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
    Request *data = NULL;
    CRequest *c_data = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        c_data = reinterpret_cast<CRequest *>(msg->data());
        data = c_data->request_;
        {
            UrlFilter *filter = c_data->url_filter_;
            filter->set_pf_list(data->prefix_filter_list_);
            filter->prepare_prefix();
            filter->filter_prefix();
            SP_DEBUG("size=%d,count=%d\n", filter->size_, filter->list_count_);
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