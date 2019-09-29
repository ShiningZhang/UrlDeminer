#pragma once
#ifndef SUPER_CODE_GLOBAL_H_
#define SUPER_CODE_GLOBAL_H_

#include "util.h"

class Buffer_Element : public SP_Data_Block
{
public:
    Buffer_Element(size_t size);
    virtual ~Buffer_Element();
    char *ptr;
    size_t length;
    size_t rd_pos;
    size_t wt_pos;
};

#endif