#include "readurllarge_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "url_filter.h"
#include "midfile.h"

#include "writeurllarge_module.h"

ReadUrlLarge_Module::ReadUrlLarge_Module(int threads)
    : threads_num_(threads)
{
}

ReadUrlLarge_Module::~ReadUrlLarge_Module()
{
}

int ReadUrlLarge_Module::open()
{
    activate(threads_num_);
    return 0;
}

void ReadUrlLarge_Module::svc()
{
    Request *data = gRequest;
    CRequest *c_data = NULL;
    SP_Message_Block_Base *msg = NULL;
    // for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        UrlPFFilter *filter = NULL;
        data = gRequest;
        MidFile *mid_file = data->mid_file_;
        data->size_split_buf = DOMAIN_CHAR_COUNT * DOMAIN_CHAR_COUNT;
        SP_DEBUG("ReadUrlLarge_Module:begin:data->size_split_buf=%d\n", data->size_split_buf);
        uint64_t read_size = 0;
        uint64_t read_pf_size = 0;
        uint64_t read_url_size = 0;
        char *buf = NULL;
        uint64_t buf_size = 0;
        int count_pf[3][2] = {0};
        int count_url = 0;
        SPFFilter *spffilter = NULL;
        SUrlFilter *surlfilter = NULL;
        for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
        {
            for (int j = 0; j < DOMAIN_CHAR_COUNT; ++j)
            {
                read_pf_size = 0;
                read_url_size = 0;
                count_url = 0;
                buf_size = 0;
                memset(count_pf, 0, 3 * 2 * sizeof(int));
                for (uint k = 0; k < mid_file->prefixfile_list_.size(); ++k)
                {
                    read_pf_size += mid_file->prefixfile_list_[k]->size2_[i][j];
                }
                for (uint k = 0; k < mid_file->largefile_list_.size(); ++k)
                {
                    read_url_size += mid_file->largefile_list_[k]->size1_[i][j];
                }
                // SP_DEBUG("[%d,%d]read_pf_size=%d,read_url_size=%d, send=%d,recv=%d\n", i, j, read_pf_size, read_url_size, data->size_split_buf, data->recv_split_);
                if (read_url_size == 0)
                {
                    for (uint k = 0; k < mid_file->prefixfile_list_.size(); ++k)
                    {
                        fseek(mid_file->prefixfile_list_[k]->fp_, (long)(mid_file->prefixfile_list_[k]->size2_[i][j]), SEEK_CUR);
                    }
                    gMCount.lock();
                    ++data->recv_split_;
                    gMCount.unlock();

                    continue;
                }
                buf = (char *)malloc((read_pf_size) * sizeof(char));
                for (uint k = 0; k < mid_file->prefixfile_list_.size(); ++k)
                {
                    FileElementPrefix *p = mid_file->prefixfile_list_[k];
                    if (p->size2_[i][j] == 0)
                        continue;
                    size_t size = fread_unlocked(buf + buf_size, 1, p->size2_[i][j], p->fp_);
                    buf_size += size;
                    for (int m = 0; m < 3; ++m)
                    {
                        for (int n = 0; n < 2; ++n)
                        {
                            count_pf[m][n] += p->count_[i][j][m][n];
                        }
                    }
                }
                {
                    unique_lock<mutex> lock(gMutexUrlPfTask);
                    while (gQUrlPfTask.empty())
                    {
                        gCVUrlPfTask.wait(lock);
                    }
                    spffilter = gQUrlPfTask.front();
                    gQUrlPfTask.pop();
                }
                for (uint k = 0; k < mid_file->prefixfile_list_.size(); ++k)
                {
                    FileElementPrefix *p = mid_file->prefixfile_list_[k];
                    memcpy(spffilter->rd_count_[k], p->count_[i][j], 3 * 2 * sizeof(int));
                }
                spffilter->load_buf(buf, read_pf_size, count_pf);
                spffilter->file_size_ = mid_file->prefixfile_list_.size();
                spffilter->send_size_ = mid_file->largefile_list_.size();
                spffilter->load_pf();
                spffilter->pre_pf();
                count_url = 0;
                buf_size = 0;
                if (read_url_size < MEM_SIZE_256)
                {
                    buf = (char *)malloc(read_url_size * sizeof(char));
                    buf_size = 0;
                    count_url = 0;
                    for (uint k = 0; k < mid_file->largefile_list_.size(); ++k)
                    {
                        if (mid_file->largefile_list_[k]->size1_[i][j] == 0)
                            continue;
                        size_t size = fread_unlocked(buf + buf_size, 1, mid_file->largefile_list_[k]->size1_[i][j], mid_file->largefile_list_[k]->fp_[i]);
                        buf_size += size;
                        count_url += mid_file->largefile_list_[k]->count1_[i][j];
                        // SP_DEBUG("[%d,%d][%d]count1_=%d\n", i, j, k, mid_file->largefile_list_[k]->count1_[i][j]);
                    }
                    get(msg);
                    surlfilter = reinterpret_cast<SUrlFilter *>(msg->data());
                    surlfilter->load_buf(buf, buf_size, count_url);
                    surlfilter->file_size_ = mid_file->largefile_list_.size();
                    surlfilter->pf_ = spffilter;
                    put_next(msg);
                }
                else
                {
                    uint start = 0;
                    uint end = 0;
                    uint file_count = mid_file->largefile_list_.size();
                    while (start < file_count)
                    {
                        buf_size = 0;
                        count_url = 0;
                        while (buf_size < MEM_SIZE_256 && end < file_count)
                        {
                            buf_size += mid_file->largefile_list_[end++]->size1_[i][j];
                        }
                        buf = (char *)malloc(buf_size * sizeof(char));
                        buf_size = 0;
                        count_url = 0;
                        for (uint k = start; k < end; ++k)
                        {
                            if (mid_file->largefile_list_[k]->size1_[i][j] == 0)
                                continue;
                            size_t size = fread_unlocked(buf + buf_size, 1, mid_file->largefile_list_[k]->size1_[i][j], mid_file->largefile_list_[k]->fp_[i]);
                            buf_size += size;
                            count_url += mid_file->largefile_list_[k]->count1_[i][j];
                        }
                        get(msg);
                        surlfilter = reinterpret_cast<SUrlFilter *>(msg->data());
                        surlfilter->load_buf(buf, buf_size, count_url);
                        surlfilter->file_size_ = end - start;
                        surlfilter->pf_ = spffilter;
                        put_next(msg);
                        start = end;
                    }
                }
            }
        }
        if (data->recv_split_ == data->size_split_buf)
        {
            SP_DEBUG("ReadUrlLarge_Module:send\n");
            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)data));
            WriteUrlLarge_Module::instance()->put_next(msg);
        }
        SP_DEBUG("ReadUrlLarge_Module:size_split_buf=%d,recv=%d,end\n", data->size_split_buf, data->recv_split_);
        gettimeofday(&t2, 0);
        SP_DEBUG("ReadUrlLarge_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("ReadUrlLarge_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int ReadUrlLarge_Module::init()
{
    return 0;
}