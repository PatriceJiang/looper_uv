#include "ThreadLoop.h"
#include <cstdlib>
namespace cocos2d
{
    namespace loop
    {
        static uv_once_t once;
        static uv_key_t key;
        static uv_mutex_t mtx;
        static void init_key()
        {
            uv_key_create(&key);
            uv_mutex_init(&mtx);
        }

        uv_loop_t * ThreadLoop::getThreadLoop()
        {
            uv_once(&once, init_key);
            uv_mutex_lock(&mtx);
            void *d = uv_key_get(&key);
            if (d == nullptr) {
                uv_loop_t *loop = uv_loop_new();
                uv_loop_init(loop);
                uv_key_set(&key, loop);
                d = loop;
            }
            uv_mutex_unlock(&mtx);
            return (uv_loop_t*)d;
        }

    }
}
