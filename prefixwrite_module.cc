#include "prefixwrite_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "prefix_filter.h"
#include "midfile.h"

PrefixWrite_Module::PrefixWrite_Module(int threads)
    : threads_num_(threads)
{
}

PrefixWrite_Module::~PrefixWrite_Module()
{
}

int PrefixWrite_Module::open()
{
    activate(threads_num_);
    return 0;
}

void PrefixWrite_Module::svc()
{
    Request *data = NULL;
    CRequest *c_data = NULL;
    MidFile *mid_file = NULL;
    int buf_size = 128 * 1024 * 1024;
    char *buf = (char *)malloc(buf_size * sizeof(char) + 2058);
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        c_data = reinterpret_cast<CRequest *>(msg->data());
        data = c_data->request_;
        SP_DES(msg);
        int idx = c_data->idx_;
        mid_file = data->mid_file_;

        PrefixFilterLargeLoad *filter = (PrefixFilterLargeLoad *)c_data->obj_;
        if (filter != NULL)
        {

            mid_file->write_prefix(filter, idx, buf, buf_size);
            delete filter;
            filter = NULL;
        }
        SP_DES(c_data);

        lock_.lock();
        data->recv_split_++;
        lock_.unlock();
        if (data->recv_split_ == data->size_split_buf)
        {
            data->reset_para();
            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)data));
            put_next(msg);
        }

        SP_DEBUG("PrefixWrite_Module:send=%d;recv=%d\n", data->size_split_buf, data->recv_split_);
        gettimeofday(&t2, 0);
        SP_DEBUG("PrefixWrite_Module=%ldms.idx=%d\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000, idx);
        //SP_LOGI("PrefixWrite_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
    delete buf;
}

int PrefixWrite_Module::init()
{
    return 0;
}