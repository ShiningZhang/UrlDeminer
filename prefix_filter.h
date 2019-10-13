#ifndef PREFIX_FILTER_H_
#define PREFIX_FILTER_H_

#include <vector>
#include <stdint.h>
#include <string>

class PrefixFilter
{
public:
    PrefixFilter();
    ~PrefixFilter();
    int filter(char *domainPort, uint16_t size) const;

public:
    void add(char *domainPortBuffer, char allow);
    void load_(char *p, uint64_t size);

    void prepare_range(char **list, int size, int *&range);
    void prepare_buf(char *p, uint64_t size);

public:
    static PrefixFilter *load(char *p, uint64_t size);
    static PrefixFilter *merge(std::vector<PrefixFilter *> prefix_filter_list);

public:
    std::vector<char *> list_str_;
    // *:0 +:1 =:2
    // +:0 -:1
    char **list_https_[3][2][2];
    // std::vector<char*> list_https_[3][2];
    uint64_t size_[3][2][2];
    char *p_;
    uint64_t buf_size_;
    int *list_range_[3][2][2];
    int list_count_[3][2][2];
};

#endif
