#ifndef PREFIX_FILTER_H_
#define PREFIX_FILTER_H_

#include <vector>
#include <stdint.h>
#include <string>

class PrefixFilter
{
public:
    PrefixFilter();
    int filter(char *domainPort, uint16_t size) const;

public:
    void add(char *domainPortBuffer, char allow);
    void load_(char *p, uint64_t size);

    void prepare_range(std::vector<char *> &list, int *&range);

public:
    static PrefixFilter *load(char *p, uint64_t size);

public:
    std::vector<char*> list_str_;
    // *:0 +:1 =:2
    // +:0 -:1
    std::vector<char*> list_https_[3][2][2];
    // std::vector<char*> list_https_[3][2];
    uint64_t size_[3][2][2];
    char * p_;
    uint64_t buf_size_;
    int *list_range_[3][2][2];
    // * +
    std::vector<char *> list_inpass_http_;
    std::vector<char *> list_inpass_https_;
    char *p_inpass_;
    uint32_t size_inpass_;
    // * -
    std::vector<char *> list_inhit_http_;
    std::vector<char *> list_inhit_https_;
    char *p_inhit_;
    uint32_t size_inhit_;
    // + +
    std::vector<char *> list_expass_http_;
    std::vector<char *> list_expass_https_;
    char *p_expass_;
    uint32_t size_expass_;
    // + -
    std::vector<char *> list_exhit_http_;
    std::vector<char *> list_exhit_https_;
    char *p_exhit_;
    uint32_t size_exhit_;
    // = +
    std::vector<char *> list_eqpass_http_;
    std::vector<char *> list_eqpass_https_;
    char *p_eqpass_;
    uint32_t size_eqpass_;
    // = -
    std::vector<char *> list_eqhit_http_;
    std::vector<char *> list_eqhit_https_;
    char *p_eqhit_;
    uint32_t size_eqhit_;
};

#endif
