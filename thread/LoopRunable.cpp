#include "LoopRunable.h"

#include <cassert>

LoopRunable::LoopRunable(uv_loop_t *loop, Loop *tsk, milliseconds interval) :
    _uvLoop(loop), _task(tsk), _intervalMS(duration_cast<milliseconds>(interval).count())
{
    uv_timer_init(loop, &_uvTimer);
    _uvTimer.data = this;
}

void LoopRunable::beforeRun()
{
    if(_task)
        _task->before();
    _startTime = high_resolution_clock::now();
    _updateTimes = 1;
    scheduleTaskUpdate();
}

void LoopRunable::run()
{
    //schedule update for task
    uv_run(_uvLoop, UV_RUN_DEFAULT);
}

void LoopRunable::afterRun()
{
    if(_task)
        _task->after();
    uv_loop_close(_uvLoop);
}

static void timer_handle(uv_timer_t *timer)
{
    LoopRunable *self = (LoopRunable*)timer->data;
    self->onTimer();
    self->scheduleTaskUpdate();
}

void LoopRunable::onTimer()
{
    _updateTimes += 1;
    if(_task)
        _task->update((int)_intervalMS);
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
    uv_timer_start(&_uvTimer, timer_handle, delay, 0);
}