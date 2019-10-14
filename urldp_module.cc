#include "urldp_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "url_filter.h"

UrlDP_Module::UrlDP_Module(int threads)
    : threads_num_(threads)
{
}

UrlDP_Module::~UrlDP_Module()
{
}

int UrlDP_Module::open()
{
    activate(threads_num_);
    return 0;
}

void UrlDP_Module::svc()
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
        c_data = reinterpret_cast<CRequest *>(msg->data());
        data = c_data->request_;
        {
            UrlFilter *filter = c_data->url_filter_;
            filter->load1();
            filter->set_dp_list(data->domain_filter_list_);
            filter->filter_domainport();
        }

        put_next(msg);

        gettimeofday(&t2, 0);
        SP_DEBUG("UrlDP_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("UrlDP_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int UrlDP_Module::init()
{
    return 0;
}