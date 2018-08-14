#ifndef __THREAD_LOOP_H__
#define __THREAD_LOOP_H__

#include <uv.h>


class ThreadLoop {
public:
    static uv_loop_t * getThreadLoop();
};

#endif