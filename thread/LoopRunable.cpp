#include "LoopRunable.h"

#include <cassert>

LoopRunable::LoopRunable(uv_loop_t *loop, std::shared_ptr<Loop> tsk, milliseconds interval) :
    loop(loop), task(tsk), intervalMS(duration_cast<milliseconds>(interval).count())
{
    uv_timer_init(loop, &timer);
    timer.data = this;
}

void LoopRunable::beforeRun()
{
    task->before();
    startTime = high_resolution_clock::now();
    updateTimes = 1;
    scheduleTaskUpdate();
}

void LoopRunable::run()
{
    //schedule update for task
    uv_run(loop, UV_RUN_DEFAULT);
}

void LoopRunable::afterRun()
{
    task->after();
    uv_loop_close(loop);
}

static void timer_handle(uv_timer_t *timer)
{
    LoopRunable *self = (LoopRunable*)timer->data;
    self->onTimer();
    self->scheduleTaskUpdate();
}

void LoopRunable::onTimer()
{
    updateTimes += 1;
    task->update((int)intervalMS);
}

void LoopRunable::scheduleTaskUpdate()
{
    auto now = high_resolution_clock::now();
    while (expectTime() <= now) {
        onTimer();
    }
    //auto diff = expectTime() - high_resolution_clock::now();
    auto diff = expectTime() - now;
    auto delay = duration_cast<milliseconds>(diff).count();
    assert(delay >= 0);
    uv_timer_start(&timer, timer_handle, delay, 0);
}