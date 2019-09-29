#ifndef SP_SINGLETON_H
#define SP_SINGLETON_H

#pragma once

template <class TYPE>
class SP_Singleton
{
public:
    static TYPE *instance(void);
    static void close(void);
protected:
    SP_Singleton() {};
    SP_Singleton(const SP_Singleton&);
    SP_Singleton& operator = (const SP_Singleton&);

    TYPE instance_;
    static SP_Singleton<TYPE> *singleton_;
    static SP_Singleton<TYPE> *&instance_i(void);
};

#include <mutex>
template <class TYPE> SP_Singleton<TYPE> *
SP_Singleton<TYPE>::singleton_ = 0;

template<class TYPE> SP_Singleton<TYPE> *&
SP_Singleton<TYPE>::instance_i(void)
{
    return SP_Singleton<TYPE>::singleton_;
}

template<class TYPE> TYPE *
SP_Singleton<TYPE>::instance(void)
{
    SP_Singleton<TYPE> *&singleton =
        SP_Singleton<TYPE>::instance_i();
    if (singleton == 0)
    {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock(mutex);
        if (singleton == 0)
        {
            singleton = new SP_Singleton<TYPE>;
        }
    }
    return &singleton->instance_;
}

template<class TYPE> void
SP_Singleton<TYPE>::close(void)
{
    if (singleton_)
    {
        delete singleton_;
        singleton_ = 0;
    }
}
#endif