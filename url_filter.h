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
    int load2(char *p, uint size, int type, int count);
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

class UrlFilterLarge : public UrlFilter
{
public:
    UrlFilterLarge();
    ~UrlFilterLarge();
    void filter_domainport_large();
    void prepare_write();

    char **list_write_[DOMAIN_CHAR_COUNT][DOMAIN_CHAR_COUNT];
    int list_write_count_[DOMAIN_CHAR_COUNT][DOMAIN_CHAR_COUNT];
};

#include "SP_Data_Block.h"

class UrlPFFilter : public SP_Data_Block
{
public:
    UrlPFFilter();
    virtual ~UrlPFFilter();
    void load_buf(char *buf, uint64_t buf_size, uint64_t pf_size, uint64_t url_size, int count[3][2], int count_url);
    void release_buf();
    int load_pf();
    int load_url();
    void pre_pf();
    void prepare_range(char **list, int size, int *&range);
    void pre_url();
    void filter();
    int write_tag(FILE *fp);

public:
    char *buf_;
    uint64_t buf_size_;
    uint64_t pf_size_;
    uint64_t url_size_;
    int count_[3][2];
    int rd_count_[16][3][2];
    int file_size_;
    char **pf_list_[3][2][2];
    int pf_count_[3][2][2];
    int *pf_range_[3][2][2];
    int pf_len_;
    char *pf_buf_;
    char *url_buf_;
    vector<char *> list_str_;
    char **url_list_;
    int url_count_;
    FilterCounters counters_;
    char *out_;
    uint64_t out_size_;
    int out_offset_;
};

#endif
