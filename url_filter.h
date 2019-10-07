#ifndef URL_FILTER_H_
#define URL_FILTER_H_

#include <vector>
#include <stdint.h>
#include <string>

#include "domain_filter.h"
#include "util.h"

class DomainFilter;
class PrefixFilter;

class URLFilter
{
public:
    int filter(char *domainPort, uint16_t size) const;

public:
    void add(char *domainPortBuffer, char allow);
    void load_(char *p, uint64_t size);

public:
    static URLFilter *load(char *p, uint64_t size);

    void filter_domainport();
    void filter_prefix();

public:
    char *p_;
    uint64_t size_;
    std::vector<char *> list_;
    std::vector<DomainPortBuf> list_domainport_;
    FilterCounters counters_;
    std::vector<DomainFilter *> list_domainfilter_;
    std::vector<PrefixFilter *> list_prefixfilter_;
};

#endif
