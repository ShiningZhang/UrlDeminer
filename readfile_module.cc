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
    char *buf;
    Request *data = NULL;
    CRequest *c_data = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        data = reinterpret_cast<Request *>(msg->data());
        SP_DES(msg);
        uint64_t length = data->length_;
        uint64_t size = 0;
        uint64_t begin = 0;
        uint64_t line_size = length < 1024 * 8 ? length : ceil((double)length / 8) + 64;
        SP_DEBUG("ReadFile_Module:length=%lld,line_size=%zu\n", length, line_size);
        data->size_split_buf = (int)ceil((double)length / line_size);
        int split = 0;
        while (begin < length)
        {
            if (begin + line_size > length)
                line_size = length - begin;
            buf = NULL;
            buf = (char *)malloc((line_size + BUFHEADSIZE) * sizeof(char));
            if (buf == NULL)
            {
                fputs("Memory error", stderr);
                exit(2);
            }
            buf = buf + BUFHEADSIZE;
            // flockfile(data->fp_in_);
            SP_DEBUG("fp=%p,buf=%p,line_size=%zu\n", data->fp_in_, buf, line_size);
            size = readcontent(data->fp_in_, buf, line_size);
            SP_DEBUG("size=%d\n", size, buf);
            begin += size;

            // funlockfile(data->fp_in_);
            SP_NEW(c_data, CRequest(data));
            c_data->buffer_ = buf;
            c_data->size_ = size;

            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)c_data));
            ++split;
            put_next(msg);
        }
        data->is_read_end_ = true;
        for (int i = split; i < data->size_split_buf; ++i)
        {
            SP_NEW(c_data, CRequest(data));
            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)c_data));
            put_next(msg);
        }
        SP_DEBUG("ReadFile_Module:size_split_buf=%d,end\n", data->size_split_buf);
        gettimeofday(&t2, 0);
        SP_DEBUG("ReadFile_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("ReadFile_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int ReadFile_Module::init()
{
    return 0;
}