#ifndef MID_FILE_H_
#define MID_FILE_H_

#include <vector>
#include "util.h"

class PrefixFilterMerge;
class PrefixFilterLargeLoad;

class MidFile
{
public:
    MidFile();
    ~MidFile();
    int init();
    void uninit();
    int write_buf(char *p, int size);
    int write_mid(char ***p, int *size, int idx);
    int write_mid_large(char **p[DOMAIN_CHAR_COUNT][DOMAIN_CHAR_COUNT], int size[DOMAIN_CHAR_COUNT][DOMAIN_CHAR_COUNT], int idx);
    void sort_file_list();

    int write_prefix(PrefixFilterLargeLoad *filter, int idx);

    std::vector<FileElement *> file_list_;
    std::vector<FileElementLarge *> largefile_list_;
    int buf_size_;
    char *buf_;
    int wt_size_;

    std::vector<FileElementPrefix *> prefixfile_list_;
};
#endif