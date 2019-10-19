#include "readurllarge_module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "util.h"
#include <string.h>
#include <math.h>

#include "url_filter.h"
#include "midfile.h"

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
    Request *data = NULL;
    CRequest *c_data = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2, start;
        gettimeofday(&start, 0);
        UrlPFFilter *filter = NULL;
        data = reinterpret_cast<Request *>(msg->data());
        SP_DES(msg);
        MidFile *mid_file = data->mid_file_;
        data->size_split_buf = DOMAIN_CHAR_COUNT * DOMAIN_CHAR_COUNT;
        uint64_t read_size = 0;
        uint64_t read_pf_size = 0;
        uint64_t read_url_size = 0;
        char *buf = NULL;
        uint64_t buf_size = 0;
        int count_pf[3][2] = {0};
        int count_url = 0;
        for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
        {
            for (int j = 0; j < DOMAIN_CHAR_COUNT; ++j)
            {
                read_pf_size = 0;
                read_url_size = 0;
                count_url = 0;
                for (int k = 0; k < mid_file->prefixfile_list_.size(); ++k)
                {
                    read_pf_size += mid_file->prefixfile_list_[k]->size2_[i][j];
                }
                for (int k = 0; k < mid_file->largefile_list_.size(); ++k)
                {
                    read_url_size += mid_file->largefile_list_[k]->size1_[i][j];
                }
                if (read_url_size == 0)
                {
                    gMCount.lock();
                    ++data->recv_split_;
                    gMCount.unlock();
                    continue;
                }
                for (int k = 0; k < mid_file->prefixfile_list_.size(); ++k)
                {
                    FileElementPrefix *p = mid_file->prefixfile_list_[k];
                    uint64_t size = readcontent_unlocked1(p->fp_, buf + buf_size, p->size2_[i][j]);
                    buf_size += size;
                    for (int m = 0; m < 3; ++m)
                    {
                        for (int n = 0; n < 2; ++n)
                        {
                            count_pf[m][n] += p->count_[i][j][m][n];
                        }
                    }
                }
                for (int k = 0; k < mid_file->largefile_list_.size(); ++k)
                {
                    uint64_t size = readcontent_unlocked1(mid_file->largefile_list_[k]->fp_[i], buf + buf_size, mid_file->largefile_list_[k]->size1_[i][j]);
                    buf_size += size;
                    count_url += mid_file->largefile_list_[k]->count1_[i][j];
                }
                {
                    unique_lock<mutex> lock(gMutex);
                    while (gQueueFilter.empty())
                    {
                        gCV.wait(lock);
                    }
                    filter = gQueueFilter.front();
                    gQueueFilter.pop();
                }
                filter->load_buf(buf, buf_size, read_pf_size, read_url_size, count_pf, count_url);
                filter->rd_count_ = (int *)malloc(mid_file->prefixfile_list_.size() * 6);
                for (int k = 0; k < mid_file->prefixfile_list_.size(); ++k)
                {
                    FileElementPrefix *p = mid_file->prefixfile_list_[k];
                    for (int m = 0; m < 3; ++m)
                    {
                        for (int n = 0; n < 2; ++n)
                        {
                            filter->rd_count_[k * 6 + m * 2 + n] = p->count_[i][j][m][n];
                        }
                    }
                }
                filter->pf_len_ = mid_file->prefixfile_list_.size();
                memset(count_pf, 0, 3 * 2 * sizeof(int));
                {
                    unique_lock<mutex> lock1(gMutexUrlPfTask);
                    gQUrlPfTask.push(filter);
                    gCVUrlPfTask.notify_one();
                }
            }
        }
        SP_DEBUG("ReadUrlLarge_Module:size_split_buf=%d,end\n", data->size_split_buf);
        gettimeofday(&t2, 0);
        SP_DEBUG("ReadUrlLarge_Module=%ldms.\n", (t2.tv_sec - start.tv_sec) * 1000 + (t2.tv_usec - start.tv_usec) / 1000);
        //SP_LOGI("ReadUrlLarge_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int ReadUrlLarge_Module::init()
{
    return 0;
}