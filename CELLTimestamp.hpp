#ifndef _CELLTimestamp_hpp_
#define _CELLTimestamp_hpp_


#include <chrono>
using namespace std::chrono;

class CELLTime
{
public:
    //当前时间戳 （毫秒）
    static time_t getNowInMilliSec()
    {
       return duration_cast<milliseconds>
                (high_resolution_clock::now().time_since_epoch()).count();
    }
};

class CELLTimestamp
{
public:
    CELLTimestamp()
    {
        update();
    }

    ~CELLTimestamp()
    {}

    void update()
    {
        _begin = high_resolution_clock::now();
    }

    //秒
    double getElapsedSecond()
    {
        return getElapseTimeInMicroSec() * 0.000001;
    }
    //毫秒
    double getElapsedTimeInMilliSec()
    {
        return getElapseTimeInMicroSec() * 0.001;    
    }
    //微秒
    long long getElapseTimeInMicroSec()
    {
        return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
    }

protected:
    time_point<high_resolution_clock> _begin;
};


#endif