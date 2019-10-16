#ifndef URL_FILTER_H_
#define URL_FILTER_H_

#include <vector>
#include <stdint.h>
#include <string>

#include "domain_filter.h"
#include "util.h"

class DomainFilter;
class PrefixFilter;

struct stDPRES
{
    uint16_t n;
    uint16_t port;
    int ret;
};

class UrlFilter
{
public:
    UrlFilter();
    virtual ~UrlFilter();
    int filter(char *domainPort, uint16_t size) const;

public:
    void add(char *domainPortBuffer, char allow);
    int load_(char *p, uint64_t size);
    int load1();
    // int load2(char *p, uint size);
    int load2(char *p, uint size, int type);
    // int load2_(char *p, uint64_t size, int count);

public:
    static UrlFilter *load(char *p, uint64_t size);

    void filter_domainport();
    void filter_prefix();
    void prepare_prefix();

    int write_tag(FILE *fp);
    void set_dp_list(vector<DomainFilter *> &list);
    void set_pf_list(vector<PrefixFilter *> &list);

    int prepare_buf(char *p, uint64_t size);

    void clear_para();
    void clear_counter();
    void clear_domain_list();

public:
    char *p_;
    uint64_t size_;
    uint64_t buf_size_;
    // std::vector<char *> list_;
    // std::vector<DomainPortBuf> list_domainport_;
    FilterCounters counters_;
    std::vector<DomainFilter *> list_domainfilter_;
    DomainFilter *domainfilter_;
    std::vector<PrefixFilter *> list_prefixfilter_;
    PrefixFilter *prefixfilter_;
    char *out_;
    uint64_t out_size_;
    int out_offset_;

    char **list_[DOMAIN_CHAR_COUNT];
    int list_count_[DOMAIN_CHAR_COUNT];
    int max_list_count_[DOMAIN_CHAR_COUNT];
    DomainPortBuf *list_domainport_[DOMAIN_CHAR_COUNT];
    int list_domainport_count_[DOMAIN_CHAR_COUNT];
    int max_list_domainport_count_[DOMAIN_CHAR_COUNT];
};

#endif
