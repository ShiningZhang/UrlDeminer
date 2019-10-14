#ifndef URL_FILTER_H_
#define URL_FILTER_H_

#include <vector>
#include <stdint.h>
#include <string>

#include "domain_filter.h"
#include "util.h"

class DomainFilter;
class PrefixFilter;

class UrlFilter
{
public:
    int filter(char *domainPort, uint16_t size) const;

public:
    void add(char *domainPortBuffer, char allow);
    int load_(char *p, uint64_t size);
    int load1();
    int load2(char *p, int size);
    int load2_(char *p, uint64_t size, char **list);

public:
    static UrlFilter *load(char *p, uint64_t size);

    void filter_domainport();
    void filter_prefix();
    void prepare_prefix();

    int write_tag(FILE *fp);
    void set_dp_list(vector<DomainFilter *> &list);
    void set_pf_list(vector<PrefixFilter *> &list);

    int prepare_buf(char *p, uint64_t size);

public:
    char *p_;
    uint64_t size_;
    // std::vector<char *> list_;
    // std::vector<DomainPortBuf> list_domainport_;
    FilterCounters counters_;
    std::vector<DomainFilter *> list_domainfilter_;
    std::vector<PrefixFilter *> list_prefixfilter_;
    char *out_;
    uint64_t out_size_;
    int out_offset_;

    char **list_;
    int list_count_;
    int max_list_count_;
    DomainPortBuf *list_domainport_;
    int list_domainport_count_;
    int max_list_domainport_count_;
};

#endif
