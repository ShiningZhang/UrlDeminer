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
                SP_DEBUG("[%d,%d]read_pf_size=%d,read_url_size=%d\n", i, j, read_pf_size, read_url_size);
                if (read_url_size == 0)
                {
                    gMCount.lock();
                    ++data->recv_split_;
                    gMCount.unlock();
                    continue;
                }
                buf = (char *)malloc((read_pf_size + read_url_size) * sizeof(char));
                for (int k = 0; k < mid_file->prefixfile_list_.size(); ++k)
                {
                    FileElementPrefix *p = mid_file->prefixfile_list_[k];
                    if (p->size2_[i][j] == 0)
                        continue;
                    size_t size = fread_unlocked(buf + buf_size, 1, p->size2_[i][j], p->fp_);
                    fprintf(stderr, "[%d,%d,%d]size=%d,prefixfile:%s\n", i, j, k, size, buf + buf_size + 3);
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
                    if (mid_file->largefile_list_[k]->size1_[i][j] == 0)
                        continue;
                    size_t size = fread_unlocked(buf + buf_size, 1, mid_file->largefile_list_[k]->size1_[i][j], mid_file->largefile_list_[k]->fp_[i]);
                    fprintf(stderr, "largefile:%s\n", buf + buf_size + 3);
                    buf_size += size;
                    count_url += mid_file->largefile_list_[k]->count1_[i][j];
                }
                get(msg);
                filter = reinterpret_cast<UrlPFFilter *>(msg->data());

                filter->load_buf(buf, buf_size, read_pf_size, read_url_size, count_pf, count_url);
                filter->file_size_ = mid_file->prefixfile_list_.size();
                for (int k = 0; k < mid_file->prefixfile_list_.size(); ++k)
                {
                    FileElementPrefix *p = mid_file->prefixfile_list_[k];
                    memcpy(filter->rd_count_[k], p->count_[i][j], 3 * 2 * sizeof(int));
                }
                filter->pf_len_ = mid_file->prefixfile_list_.size();
                memset(count_pf, 0, 3 * 2 * sizeof(int));
                put_next(msg);
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