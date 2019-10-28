#include "midfile.h"
#include "util.h"

#include "pdqsort.h"

#include "prefix_filter.h"
#include "url_filter.h"

MidFile::MidFile()
{
}

MidFile::~MidFile()
{
    if (buf_ != NULL)
    {
        free(buf_);
        buf_ = NULL;
    }
    buf_size_ = 0;
    for (uint i = 0; i < file_list_.size(); ++i)
    {
        FileElement *e = file_list_[i];
        for (int j = 0; j < DOMAIN_CHAR_COUNT; ++j)
        {
            if (e->fp_[j] != NULL)
                fclose(e->fp_[j]);
        }
        delete e;
    }
    for (uint a = 0; a < prefixfile_list_.size(); ++a)
    {
        FileElementPrefix *e = prefixfile_list_[a];
        {
            if (e->fp_ != NULL)
                fclose(e->fp_);
        }
        delete e;
    }
    for (uint a = 0; a < largefile_list_.size(); ++a)
    {
        FileElementLarge *e = largefile_list_[a];
        for (int j = 0; j < DOMAIN_CHAR_COUNT; ++j)
        {
            if (e->fp_[j] != NULL)
                fclose(e->fp_[j]);
        }
        delete e;
    }
}

int MidFile::init()
{
    buf_size_ = FILESPLITSIZE;
    buf_ = (char *)malloc(FILESPLITSIZE * sizeof(char) + 2058);
    return buf_size_;
}

void MidFile::uninit()
{
    if (buf_ != NULL)
    {
        free(buf_);
        buf_ = NULL;
    }
    buf_size_ = 0;
}

static bool cmp_file_element(const FileElement *e1, const FileElement *e2)
{
    return e1->idx < e2->idx;
}

void MidFile::sort_file_list()
{
    pdqsort(file_list_.begin(), file_list_.end(), cmp_file_element);
}

inline int write_(char **p, int &begin, int end, int buf_size, char *&buf)
{
    int offset = 0;
    char *pa = NULL;
    int na = 0;
    do
    {
        pa = p[begin];
        na = (int)*((uint16_t *)pa);
        na += 14; //3+(len+1)+1+8+1
        memcpy(buf + offset, pa - 1, na);
        offset += na;
        ++begin;
    } while (offset < buf_size && begin < end);
    return offset;
}

int MidFile::write_mid(char ***p, int *size, int idx)
{
    char tmp_char[32];
    FileElement *file = new FileElement;
    file->idx = idx;
    file->total_size_ = 0;
    memset(file->fp_, 0, DOMAIN_CHAR_COUNT * sizeof(FILE *));
    memset(file->size_, 0, DOMAIN_CHAR_COUNT * sizeof(size_t));
    memset(file->count_, 0, DOMAIN_CHAR_COUNT * sizeof(int));
    for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
    {
        if (size[i] == 0)
            continue;
        sprintf(tmp_char, "%d_%d", idx, i);
        FILE *fp = fopen(tmp_char, "wb+");
        size_t file_size = 0;
        int begin = 0;
        int end = size[i];
        do
        {
            int size1 = write_(p[i], begin, end, buf_size_, buf_);
            fwrite(buf_, size1, 1, fp);
            file_size += size1;
        } while (begin < end);
        rewind(fp);
        file->fp_[i] = fp;
        file->size_[i] = file_size;
        file->total_size_ += file_size;
        file->count_[i] = size[i];
    }
    file_list_.push_back(file);
    return 0;
}

inline int write_large_(char **p, int &begin, int end, int buf_size, char *&buf)
{
    int offset = 0;
    char *pa = NULL;
    int na = 0;
    do
    {
        pa = p[begin];
        na = (int)*((uint16_t *)pa);
        *(uint16_t *)(pa + 2) = na;
        pa[1] = pa[-1];
        na += 13; //3+(len+2)+1+8+1-2
        memcpy(buf + offset, pa + 1, na);
        offset += na;
        ++begin;
    } while (offset < buf_size && begin < end);
    return offset;
}

int MidFile::write_mid_large(UrlFilterLarge *filter, int size[DOMAIN_CHAR_COUNT][DOMAIN_CHAR_COUNT], int idx)
{
    char tmp_char[32];
    FileElementLarge *file = new FileElementLarge;
    file->idx = idx;
    file->total_size_ = 0;
    memset(file->fp_, 0, DOMAIN_CHAR_COUNT * sizeof(FILE *));
    memset(file->size_, 0, DOMAIN_CHAR_COUNT * sizeof(size_t));
    memset(file->count_, 0, DOMAIN_CHAR_COUNT * sizeof(int));
    memset(file->count1_, 0, DOMAIN_CHAR_COUNT * sizeof(int) * DOMAIN_CHAR_COUNT);
    memset(file->size1_, 0, DOMAIN_CHAR_COUNT * sizeof(size_t) * DOMAIN_CHAR_COUNT);
    for (int i = 0; i < DOMAIN_CHAR_COUNT; ++i)
    {
        sprintf(tmp_char, "%d_%d", idx, i);
        FILE *fp = fopen(tmp_char, "wb+");
        for (int j = 0; j < DOMAIN_CHAR_COUNT; ++j)
        {
            int count = filter->list_count_[i][j];
            if (count == 0)
                continue;
            char **p = filter->list_[i][j];
            size_t file_size = 0;
            int begin = 0;
            int end = count;
            do
            {
                int size1 = write_large_(p, begin, end, buf_size_, buf_);
                fwrite(buf_, size1, 1, fp);
                file_size += size1;
            } while (begin < end);
            file->count1_[i][j] = count;
            file->size1_[i][j] = file_size;
            file->size_[i] += file_size;
            file->total_size_ += file_size;
            file->count_[i] += count;
        }
        rewind(fp);
        file->fp_[i] = fp;
    }
    largefile_list_.push_back(file);
    return 0;
}

inline int write_prefix_(char **p, int &begin, int end, int buf_size, char *&buf)
{
    int offset = 0;
    char *pa = NULL;
    int na = 0;
    do
    {
        pa = p[begin];
        na = (int)*((uint16_t *)pa);
        na += 3; //3+(len+2)
        memcpy(buf + offset, pa - 1, na);
        offset += na;
        ++begin;
    } while (offset < buf_size && begin < end);
    return offset;
}

int MidFile::write_prefix(PrefixFilterLargeLoad *filter, int idx, char *buf, int buf_size)
{
    char tmp_char[64];
    FileElementPrefix *file = new FileElementPrefix;
    file->idx_ = idx;
    file->wt_size_ = 0;
    file->rd_size_ = 0;
    memset(file->size1_, 0, DOMAIN_CHAR_COUNT * sizeof(size_t));
    memset(file->size2_, 0, DOMAIN_CHAR_COUNT * sizeof(size_t) * DOMAIN_CHAR_COUNT);
    memset(file->count_, 0, DOMAIN_CHAR_COUNT * sizeof(int) * 3 * 2 * DOMAIN_CHAR_COUNT);
    sprintf(tmp_char, "pf_%d", idx);
    FILE *fp = fopen(tmp_char, "wb+");
    for (int m = 0; m < DOMAIN_CHAR_COUNT; ++m)
    {
        for (int n = 0; n < DOMAIN_CHAR_COUNT; ++n)
        {
            for (int i = 0; i < 3; ++i)
            {
                for (int j = 0; j < 2; ++j)
                {
                    size_t file_size = 0;
                    int begin = 0;
                    int end = filter->list_count_large_[i][j][m][n];
                    char **p = filter->list_https_large_[i][j][m][n];
                    if (begin < end)
                    {
                        do
                        {
                            int size = write_prefix_(p, begin, end, buf_size, buf);
                            fwrite(buf, size, 1, fp);
                            file_size += size;
                        } while (begin < end);
                    }
                    file->size2_[m][n] += file_size;
                    file->count_[m][n][i][j] = end;
                }
            }
            file->size1_[m] += file->size2_[m][n];
        }
    }
    rewind(fp);
    file->fp_ = fp;
    lock_.lock();
    prefixfile_list_.push_back(file);
    lock_.unlock();
    return 0;
}