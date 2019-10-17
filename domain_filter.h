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
	virtual ~DomainFilter();
	int filter(char *domainPort, uint16_t size) const;

public:
	void add(char *domainPortBuffer, char allow);
	void load_(char *p, uint64_t size);
	void prepare_buf(char *p, uint64_t size);

public:
	static DomainFilter *load(char *p, uint64_t size);

public:
	// std::vector<DomainPortBuf> list_;
	char **list_[2][DOMAIN_CHAR_COUNT];
	int list_count_[2][DOMAIN_CHAR_COUNT];
	char *p_;
	uint32_t size_;
	int *list_range_[2][DOMAIN_CHAR_COUNT];
	DomainPortBuf *list_port_[2];
	int list_port_count_[2];
};

class DomainFilterMerge : public DomainFilter
{
public:
	DomainFilterMerge();
	virtual ~DomainFilterMerge();
	int merge(vector<DomainFilter *> domain_filter_list, int type);
	int merge(vector<DomainFilter *> domain_filter_list, int type, int i);

	int merge_port(vector<DomainFilter *> list);
	int merge_port(vector<DomainFilter *> list, int i);

	void cpy_filter_list(vector<DomainFilter *> &list);
	DomainPortBuf *port_start_[2][65536];
	int port_size_[2][65536];
	int *port_range_[2][65536];
	std::vector<char *> p_list_;
	std::vector<int> buf_size_list_;
};

#endif