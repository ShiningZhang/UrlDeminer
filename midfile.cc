#include "midfile.h"
#include "util.h"

#include "pdqsort.h"

MidFile::MidFile()
{
}

MidFile::~MidFile()
{
}

int MidFile::init()
{
    buf_size_ = FILESPLITSIZE;
    buf = (char *)malloc(FILESPLITSIZE * sizeof(char));
}

static bool cmp_file_element(const FileElement *e1, const FileElement *e2)
{
    return e1->idx < e2->idx;
}

void MidFile::sort_file_list()
{
    pdqsort(file_list_.begin(), file_list_.end(), cmp_file_element);
}

int MidFile::write_mid(char **p, int size, int &buf_size, char *&buf, int in_size, int idx)
{
    char *pa = NULL;
    int na = 0;
    int offset = 0;
    for (int i = 0; i < size; ++i)
    {
        pa = p[i];
        na = (int)*((uint16_t *)pa);
        na += 13; //3+len+1+8+1
        memcpy(buf + offset, pa - 1, na);
        offset += na;
    }
    wt_size_ = offset;
    char tmp_char[32];
    sprintf(tmp_char, "%d", idx);
    FileElement *file = new FileElement;
    file->fp_ = fopen(tmp_char, "wb+");
    file->size_ = offset;
    file->idx = idx;
    fwrite(buf, offset, 1, file->fp_);
    rewind(file->fp_);
    file_list_.push_back(file);
    return offset;
}