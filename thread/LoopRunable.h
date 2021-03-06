#pragma once

#include <memory>
#include <chrono>
#include "uv.h"

#include "Loop.h"

namespace cocos2d
{
    namespace loop
    {

        using namespace std::chrono;

        class LoopRunable {
        public:
            LoopRunable(uv_loop_t *loop, Loop *tsk, milliseconds interval);
            void beforeRun();
            void run();
            void afterRun();
            void scheduleTaskUpdate();
            void onTimer();
            time_point<high_resolution_clock> expectTime()
            {
                return _startTime + milliseconds(_intervalMS * _updateTimes);
            }
        private:
            Loop * _task = nullptr;
            uv_loop_t *_uvLoop = nullptr;
            uv_timer_t _uvTimer;
            int64_t _intervalMS = 3000LL;
            time_point<high_resolution_clock> _startTime;
            int64_t _updateTimes = 0LL;
        };
    }
}

