#ifndef __CC_LOOP_RUNABL_H__
#define __CC_LOOP_RUNABL_H__

#include <memory>
#include <chrono>
#include "uv.h"

#include "Loop.h"

using namespace std::chrono;

class LoopRunable {
public:
    LoopRunable(uv_loop_t *loop, std::shared_ptr<Loop> tsk, milliseconds interval);
    void beforeRun();
    void run();
    void afterRun();

    void scheduleTaskUpdate();

    void onTimer();

    time_point<high_resolution_clock> expectTime() { return _startTime + milliseconds(_intervalMS * _updateTimes); }

private:
    std::shared_ptr<Loop> _task;
    uv_loop_t *_uvLoop;
    uv_timer_t _uvTimer;
    int64_t _intervalMS;
    time_point<high_resolution_clock> _startTime;
    int64_t _updateTimes = 0LL;

};

#endif // !__CC_LOOP_RUNABL_H__
