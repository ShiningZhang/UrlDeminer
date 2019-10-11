#pragma once
#ifndef SUPER_CODE_DOMAIN_FILTER_H_
#define SUPER_CODE_DOMAIN_FILTER_H_

#include <vector>
#include <stdint.h>

#include "util.h"

class DomainFilter
{
public:
	DomainFilter();
	~DomainFilter();
	int filter(char *domainPort, uint16_t size) const;

public:
	void add(char *domainPortBuffer, char allow);
	void load_(char *p, uint64_t size);
	void prepare_buf(char *p, uint64_t size);

public:
	static DomainFilter *load(char *p, uint64_t size);

public:
	// std::vector<DomainPortBuf> list_;
	char **list_[2][65536];
	int list_count_[2][65536];
	char *p_;
	uint32_t size_;
};

#endif