#include "SP_Message_Block_Base.h"
#include "SP_Data_Block.h"
#include "Global_Macros.h"

SP_Message_Block_Base::SP_Message_Block_Base(void *data, int idx)
    :msg_type_(Message_Type::MB_DATA),
    delete_data_(false),
    next_(0),
    msg_no_(0),
    data_((SP_Data_Block *)data),
    idx_(idx)
{

}

SP_Message_Block_Base::~SP_Message_Block_Base()
{
    if(delete_data_)
        SP_DES(data_);
}

void SP_Message_Block_Base::msg_type(unsigned int msg_type)
{
    this->msg_type_ = msg_type;
}

unsigned int SP_Message_Block_Base::msg_type()
{
    return this->msg_type_;
}

void SP_Message_Block_Base::delete_data(bool delete_data)
{
    this->delete_data_ = delete_data;
}

bool SP_Message_Block_Base::delete_data()
{
    return delete_data_;
}

void SP_Message_Block_Base::next(SP_Message_Block_Base* next)
{
    this->next_ = next;
}

SP_Message_Block_Base* SP_Message_Block_Base::next()
{
    return this->next_;
}

void SP_Message_Block_Base::data(SP_Data_Block *data)
{
    this->data_ = data;
}

SP_Data_Block *SP_Message_Block_Base::data()
{
    return this->data_;
}

void SP_Message_Block_Base::idx(int idx)
{
    this->idx_ = idx;
}

int SP_Message_Block_Base::idx()
{
    return this->idx_;
}
