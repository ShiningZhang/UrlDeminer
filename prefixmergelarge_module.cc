#include "prefixmergelarge_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "prefix_filter.h"

PrefixMergeLarge_Module::PrefixMergeLarge_Module(int threads)
    : threads_num_(threads)
{
}

PrefixMergeLarge_Module::~PrefixMergeLarge_Module()
{
}

int PrefixMergeLarge_Module::open()
{
    activate(threads_num_);
    return 0;
}

void PrefixMergeLarge_Module::svc()
{
    Request *data = NULL;
    CRequest *c_data = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        c_data = reinterpret_cast<CRequest *>(msg->data());
        data = c_data->request_;
        int idx = c_data->idx_;

        PrefixFilterMerge *filter = (PrefixFilterMerge *)data->prefix_filter_;
        if (filter != NULL)
        {
            // filter->merge_large(data->prefix_filter_list_, c_data->idx_);
        }

        put_next(msg);

        gettimeofday(&t2, 0);
        SP_DEBUG("PrefixMergeLarge_Module=%ldms.idx=%d\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000, idx);
        //SP_LOGI("PrefixMergeLarge_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int PrefixMergeLarge_Module::init()
{
    return 0;
}