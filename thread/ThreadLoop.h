#pragma once

#include <uv.h>

namespace cocos2d
{
    namespace loop
    {
        class ThreadLoop {
        public:
            static uv_loop_t * getThreadLoop();
        };
    }
}
