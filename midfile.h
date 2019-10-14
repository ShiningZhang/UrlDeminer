#ifndef MID_FILE_H_
#define MID_FILE_H_

#include <vector>
#include "util.h"

class MidFile
{
public:
    MidFile();
    ~MidFile();
    int init();
    int write_buf(char *p, int size);
    int write_mid(char **p, int size, int &buf_size, char *&buf, int in_size, int idx);
    void sort_file_list();

    std::vector<FileElement *> file_list_;
    int buf_size_;
    char *buf;
    int wt_size_;
};
#endif