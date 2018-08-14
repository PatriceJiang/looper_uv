#pragma once

#include <uv.h>


class ThreadLoop {
public:
    static uv_loop_t * getThreadLoop();
};
