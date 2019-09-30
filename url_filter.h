#ifndef URL_FILTER_H_
#define URL_FILTER_H_

#include <vector>
#include <stdint.h>
#include <string>

#include "domain_filter.h"

class URLFilter
{
public:
    int filter(char *domainPort, uint16_t size) const;

public:
    void add(char *domainPortBuffer, char allow);
    void load_(char *p, uint64_t size);

public:
    static URLFilter *load(char *p, uint64_t size);

public:
    char *p_;
    uint64_t size_;
    std::vector<char *> list_;
    std::vector<DomainPortBuf> list_domainport_;
};

#endif
