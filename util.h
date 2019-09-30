#pragma once
#ifndef SUPER_CODE_UTIL_H_
#define SUPER_CODE_UTIL_H_

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include<sys/time.h>

using namespace std;

#define FILESPLITSIZE 230000000
#define MAXSORT 8
#define VERYSMALLSIZE 1000000
#define SMALLSIZE 1100000000

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

size_t readcontent_unlocked(FILE *handle, char *p, uint64_t isize, int *end);
uint64_t sizeoffile(FILE *handle);
int setfilesplitsize(uint64_t inputfilesize, int maxsort);

#endif