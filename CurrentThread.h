#pragma once
#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread{
    extern __thread int t_cachedTid;//get the current pthread id

    void cacheTid();//if the tid is in the cache you can get it

    inline int tid(){//the api for getting a tid;
        if(__builtin_expect(t_cachedTid==0,0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}