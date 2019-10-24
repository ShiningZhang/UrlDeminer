#pragma once
#ifndef SUPER_CODE_REQUEST_H_
#define SUPER_CODE_REQUEST_H_

#include "SP_Data_Block.h"
#include "Global.h"

#include <vector>
#include <string>
#include <mutex>

#include "domain_filter.h"
#include "prefix_filter.h"
#include "midfile.h"
#include "url_filter.h"

using namespace std;

class CRequest;

class Request : public SP_Data_Block
{
public:
    Request()
        : size_split_buf(0),
          is_read_end_(false),
          recv_split_(0),
          fp_in_(NULL),
          fp_out_(NULL),
          mid_file_(NULL),
          domain_filter_(NULL),
          prefix_filter_(NULL),
          split_size_(0){};
    virtual ~Request()
    {
        for (size_t i = 0; i < domain_filter_list_.size(); ++i)
            delete domain_filter_list_[i];
        domain_filter_list_.clear();
        if (domain_filter_ != NULL)
        {
            delete (DomainFilterMerge *)domain_filter_;
            domain_filter_ = NULL;
        }
        for (size_t i = 0; i < prefix_filter_list_.size(); ++i)
            delete prefix_filter_list_[i];
        prefix_filter_list_.clear();
        if (prefix_filter_ != NULL)
        {
            delete (PrefixFilterMerge *)prefix_filter_;
            prefix_filter_ = NULL;
        }
        if (mid_file_ != NULL)
        {
            delete mid_file_;
            mid_file_ = NULL;
        }
        for (size_t i = 0; i < url_filter_list_.size(); ++i)
        {
            delete url_filter_list_[i];
        }
        url_filter_list_.clear();
    };
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
    bool is_read_end_;
    int recv_split_;
    vector<CRequest *> recv_str_list_;
    FILE *fp_in_;
    FILE *fp_out_;
    vector<DomainFilter *> domain_filter_list_;
    vector<PrefixFilter *> prefix_filter_list_;
    vector<PrefixFilterLargeLoad *> prefix_load_list_;
    vector<UrlFilter *> url_filter_list_;
    FilterCounters counter_;
    MidFile *mid_file_;
    DomainFilter *domain_filter_;
    PrefixFilter *prefix_filter_;
    uint64_t split_size_;
};

class CRequest : public SP_Data_Block
{
public:
    CRequest(Request *r)
        : request_(r), buffer_(NULL), size_(0), url_filter_(NULL), domain_filter_(NULL), obj_(NULL), idx_(0){};
    virtual ~CRequest(){};

    std::mutex lock_;
    Request *request_;
    char *buffer_;
    uint64_t begin_;
    uint64_t end_;
    uint64_t size_;
    UrlFilter *url_filter_;
    DomainFilter *domain_filter_;
    int idx_list_[3];
    void *obj_;
    int idx_;
};

#endif