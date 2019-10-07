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

class Request : public SP_Data_Block
{
public:
    Request()
        : size_split_buf(0),
          is_read_end_(false),
          recv_split_(0){};
    virtual ~Request(){};
    void reset_para(){
        recv_split_ = 0;
        size_split_buf = 0;
        is_read_end_ = false;
    }

public:
    std::mutex lock_;
    char *buffer_;
    size_t begin_;
    size_t end_;
    size_t length_;
    uint8_t idx_;
    size_t count_;
    size_t size_split_buf;
    vector<CRequest *> recv_str_list_;
    FILE *fp_out_;
    FILE *fp_in_;
    bool is_read_end_;
    vector<DomainFilter *> domain_filter_list_;
    vector<PrefixFilter *> prefix_filter_list_;
    vector<UrlFilter *> url_filter_list_;
    size_t recv_split_;
    FilterCounters counter_;
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
    std::vector<uint8_t> idx_;
    Request *request_;
    vector<UrlFilter *> url_filter_list_;
};

#endif