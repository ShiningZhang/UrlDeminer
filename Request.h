#pragma once
#ifndef SUPER_CODE_REQUEST_H_
#define SUPER_CODE_REQUEST_H_

#include "SP_Data_Block.h"
#include "Global.h"

#include <vector>
#include <string>
#include <mutex>

using namespace std;

class CRequest;
class DomainFilter;
class PrefixFilter;
class UrlFilter;
class MidFile;

class Request : public SP_Data_Block
{
public:
    Request()
        : size_split_buf(0),
          is_read_end_(false),
          recv_split_(0){};
    virtual ~Request(){};
    void reset_para()
    {
        recv_split_ = 0;
        size_split_buf = 0;
        is_read_end_ = false;
    }

public:
    std::mutex lock_;
    char *buffer_;
    size_t begin_;
    size_t end_;
    uint64_t length_;
    uint8_t idx_;
    size_t count_;
    int size_split_buf;
    vector<CRequest *> recv_str_list_;
    FILE *fp_out_;
    FILE *fp_in_;
    bool is_read_end_;
    vector<DomainFilter *> domain_filter_list_;
    vector<PrefixFilter *> prefix_filter_list_;
    vector<UrlFilter *> url_filter_list_;
    int recv_split_;
    FilterCounters counter_;
    MidFile *mid_file_;
    DomainFilter *domain_filter_;
};

class CRequest : public SP_Data_Block
{
public:
    CRequest(Request *r)
        : request_(r){};
    virtual ~CRequest(){};

    std::mutex lock_;
    char *buffer_;
    size_t begin_;
    size_t end_;
    size_t size_;
    int idx_;
    Request *request_;
    UrlFilter * url_filter_;
    DomainFilter *domain_filter_;
};

#endif