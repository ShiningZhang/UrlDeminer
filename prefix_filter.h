#ifndef PREFIX_FILTER_H_
#define PREFIX_FILTER_H_

#include <vector>
#include <stdint.h>
#include <string>

#include "util.h"

class PrefixFilter
{
public:
    PrefixFilter();
    virtual ~PrefixFilter();
    int filter(char *domainPort, uint16_t size) const;

public:
    void add(char *domainPortBuffer, char allow);
    void load_(char *p, uint64_t size);

    void prepare_range(char **list, int size, int *&range);
    void prepare_buf(char *p, uint64_t size);

public:
    static PrefixFilter *load(char *p, uint64_t size);
    static PrefixFilter *load_case2(char *p, uint64_t size);
    // static PrefixFilter *merge(std::vector<PrefixFilter *> prefix_filter_list);

public:
    std::vector<char *> list_str_;
    // *:0 +:1 =:2
    // +:0 -:1
    // http:0 https:1
    char **list_https_[2][DOMAIN_CHAR_COUNT][DOMAIN_CHAR_COUNT];
    // std::vector<char*> list_https_[3][2];
    uint64_t size_[2][DOMAIN_CHAR_COUNT][DOMAIN_CHAR_COUNT];
    char *p_;
    uint64_t buf_size_;
    int *list_range_[2][DOMAIN_CHAR_COUNT][DOMAIN_CHAR_COUNT];
    int list_count_[2][DOMAIN_CHAR_COUNT][DOMAIN_CHAR_COUNT];
};

class PrefixFilterMerge : public PrefixFilter
{
public:
    PrefixFilterMerge();
    virtual ~PrefixFilterMerge();
    int merge(std::vector<PrefixFilter *> list, int idx, int idx1);
    int merge_large(std::vector<PrefixFilter *> list, int idx);
    void cpy_filter_list(std::vector<PrefixFilter *> &list);
    std::vector<char *> p_list_;
    std::vector<int> buf_size_list_;
};

class PrefixFilterLargeLoad
{
public:
    PrefixFilterLargeLoad();
    ~PrefixFilterLargeLoad();

    void load_large_(char *p, uint64_t size);
    void prepare_buf_large(char *p, uint64_t size);
    static PrefixFilterLargeLoad *load_large(char *p, uint64_t size);

public:
    char *p_;
    uint64_t buf_size_;
    char **list_https_large_[3][2][DOMAIN_CHAR_COUNT][DOMAIN_CHAR_COUNT];
    int list_count_large_[3][2][DOMAIN_CHAR_COUNT][DOMAIN_CHAR_COUNT];
    int list_cc_[3][2][2][DOMAIN_CHAR_COUNT][DOMAIN_CHAR_COUNT];
};

#endif
