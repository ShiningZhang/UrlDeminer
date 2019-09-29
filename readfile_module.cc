#include "readfile_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

ReadFile_Module::ReadFile_Module(int threads)
    : threads_num_(threads)
{
}

ReadFile_Module::~ReadFile_Module()
{
}

int ReadFile_Module::open()
{
    activate(threads_num_);
    return 0;
}

void ReadFile_Module::svc()
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
        data = reinterpret_cast<Request *>(msg->data());
        SP_DES(msg);
        int end = 0;
        uint64_t length = data->length_;
        uint32_t size = 0;
        size_t line_size = length < 1024 * 8 ? length : ceil((double)length / 8);
        while (1)
        {
            buf = (char *)malloc(line_size);
            flockfile(data->fp_in_);

            size = readcontent_unlocked(data->fp_in_, buf, line_size, &end);

            funlockfile(data->fp_in_);
            SP_NEW(c_data, CRequest(data));
            c_data->buffer_ = buf;
            c_data->size_ = size;

            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)c_data));
            ++data->size_split_buf;
            put_next(msg);
            if(end)
            {
                break;
            }
        }
        data->is_read_end_ = true;
        gettimeofday(&t2, 0);
        SP_DEBUG("ReadFile_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("ReadFile_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int ReadFile_Module::init()
{
    return 0;
}