#include "prefixloadlarge_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "prefix_filter.h"

PrefixLoadLarge_Module::PrefixLoadLarge_Module(int threads)
    : threads_num_(threads)
{
}

PrefixLoadLarge_Module::~PrefixLoadLarge_Module()
{
}

int PrefixLoadLarge_Module::open()
{
    activate(threads_num_);
    return 0;
}

void PrefixLoadLarge_Module::svc()
{
    Request *data = NULL;
    CRequest *c_data = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        c_data = reinterpret_cast<CRequest *>(msg->data());
        data = c_data->request_;

        PrefixFilterLargeLoad *filter = PrefixFilterLargeLoad::load_large(c_data->buffer_, c_data->size_);
        if (filter != NULL)
            c_data->obj_ = (void *)filter;
        put_next(msg);

        gettimeofday(&t2, 0);
        SP_DEBUG("PrefixLoadLarge_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("PrefixLoadLarge_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int PrefixLoadLarge_Module::init()
{
    return 0;
}