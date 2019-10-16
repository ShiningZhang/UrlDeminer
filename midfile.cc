#include "midfile.h"
#include "util.h"

#include "pdqsort.h"

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
}

int MidFile::init()
{
    buf_size_ = FILESPLITSIZE;
    buf_ = (char *)malloc(FILESPLITSIZE * sizeof(char));
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
            int size = write_(p[i], begin, end, buf_size_, buf_);
            fwrite(buf_, size, 1, fp);
            file_size += size;
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