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

    time_point<high_resolution_clock> expectTime() { return startTime + milliseconds(intervalMS * updateTimes); }

private:
    std::shared_ptr<Loop> task;
    uv_loop_t *loop;
    uv_timer_t timer;
    int64_t intervalMS;
    time_point<high_resolution_clock> startTime;
    int64_t updateTimes{ 0 };
};

#endif // !__CC_LOOP_RUNABL_H__
