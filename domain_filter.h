#pragma once
#ifndef SUPER_CODE_DOMAIN_FILTER_H_
#define SUPER_CODE_DOMAIN_FILTER_H_

#include <vector>
#include <stdint.h>

struct DomainPortBuf
{
	char *start;
	uint16_t port;
	uint16_t n : 15;
	uint16_t allow : 1;
};

class DomainFilter
{
public:
	int filter(char *domainPort, uint16_t size) const;

public:
	void add(char *domainPortBuffer, char allow);
	void load_(char *p, uint64_t size);

public:
	static DomainFilter *load(char *p, uint64_t size);

public:
	std::vector<DomainPortBuf> list_;
	char *p_;
	uint32_t buf_size_;
};

#endif