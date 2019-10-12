#pragma once
#ifndef SUPER_CODE_UTIL_H_
#define SUPER_CODE_UTIL_H_

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <sys/time.h>

using namespace std;

#define FILESPLITSIZE 230000000
#define MAXSORT 8
#define VERYSMALLSIZE 1000000
#define SMALLSIZE 1100000000

#define BUFHEADSIZE 10

#define LOG(format_string, ...)                                                                         \
	{                                                                                                   \
		struct timeval __val;                                                                           \
		gettimeofday(&__val, NULL);                                                                     \
		fprintf(stderr, "%ld.%03ld " format_string, __val.tv_sec, __val.tv_usec / 1000, ##__VA_ARGS__); \
	}

class DomainFilter;
class PrefixFilter;

struct GlobalST
{
	FILE *inputfilehandle;
	uint64_t inputfilesize;
	uint32_t filesplitsize;
	vector<DomainFilter *> domain_filter_list_;
	int inputend;
};

struct FilterCounters
{
	int pass;
	int hit;
	int miss;
	int invalidUrl;
	uint32_t passchecksum;
	uint32_t hitchecksum;
	FilterCounters();
};

struct DomainPortBuf
{
	char *start;
	uint16_t port;
	uint16_t n : 15;
	uint16_t hit : 1;
};

size_t readcontent_unlocked(FILE *handle, char *p, uint64_t isize, int *end);
uint64_t sizeoffile(FILE *handle);
int setfilesplitsize(uint64_t inputfilesize, int maxsort);

int cmp64val(int64_t ia, int64_t ib);
int cmpbuf_dp(const char *pa, int na, const char *pb, int nb);
bool compare_dp(const DomainPortBuf &e1, const DomainPortBuf &e2);
int cmpbuf_pf(const char *pa, int na, const char *pb, int nb);
bool compare_prefix(const char *e1, const char *e2);

void arrangesuffix(char *s, int len);

bool compare_dp_char(const char *pa, const char *pb);

bool compare_prefix_eq(const char *e1, const char *e2);

unsigned long long file_size(const char * filename );

uint64_t readcontent_unlocked1(FILE *handle, char *p, uint64_t isize);

bool compare_dp_char_eq(const char *pa, const char *pb);

#endif