#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <iostream>
#include <atomic>
#include <cstdint>

#include "uv.h"

#include "ThreadSafeCollection.h"
#include "LoopRunable.h"
#include "ThreadLoop.h"
#include "Loop.h"
#include "option.h"
#include "Finalizer.h"
#include "SeqItem.h"

#include <memory>

class LoopRunable;

enum class ThreadCategory {
    ANY_THREAD = 0,
    MAIN_THREAD = 1 << 0,
    PHYSICS_THREAD = 1 << 1,
    RENDER_THREAD = 1 << 2,
    NET_THREAD = 1 << 3,
};

template<typename LoopEvent>
static void async_handle(uv_async_t *data);

template<typename LoopEvent>
class Looper : public std::enable_shared_from_this<Looper<LoopEvent> > {
public:

    typedef std::function<void(LoopEvent&)> EventCF;
    typedef std::function<void()> DispatchF;

    static Looper *getCurrentThread();
    static void setLocalData(const std::string &name, void *data);
    static void* getLocalData(const std::string &name);

    Looper(ThreadCategory cate, std::shared_ptr<Loop> task, int64_t updateMs);
    virtual ~Looper();

    void init();

    void run();
    void asyncStop();
    bool syncStop();
    void join();
    void detach();

    void emit(const std::string &name, LoopEvent &arg);
    void on(const std::string &name, EventCF callback);
    void off(const std::string &name);

    void dispatch(DispatchF fn);
    void wait(DispatchF fn, int timeoutMS = 0);

    bool isCurrentThread() const;

    uv_loop_t *getUVLoop() { return _uvLoop };

private:
    void notify();
    void onNotify();
    void onStop();
    void onRun();


    uint64_t genSeq() { return _eventFnSeq.fetch_add(1); }
    void handleEvent(const std::string &name, LoopEvent &ev);
    void handleFn(const DispatchF &fn);

    enum ThreadCategory _category;
    std::shared_ptr<Loop> _loop;
    std::shared_ptr<LoopRunable> _task;
    static ThreadSafeMap<std::string, uv_key_t> _tlsKeyMap;
    ThreadSafeMapArray<std::string, EventCF> _callbackMap;
    ThreadSafeQueue<SeqItem<LoopEvent>> _pendingEvents;
    ThreadSafeQueue<SeqItem<DispatchF>> _pendingFns;

    std::atomic_uint64_t _eventFnSeq = 0;
    bool _forceStoped = false;
    bool _isStopped = false;
    bool _initialized = false;

    std::thread *_tid = nullptr;
    int64_t _intervalMs;

public:
    uv_loop_t *_uvLoop = nullptr;
    uv_async_t _uvAsync;
    friend class LoopMgr;
    friend void async_handle<LoopEvent>(uv_async_t * data);
};



using namespace std::chrono;

template<typename LoopEvent>
static void async_handle(uv_async_t *data)
{
    Looper<LoopEvent> *t = (Looper<LoopEvent> *)((char*)data - offsetof(Looper<LoopEvent>, _uvAsync));
    t->onNotify();
}


//declare static field
template<typename LoopEvent>
ThreadSafeMap<std::string, uv_key_t> Looper<LoopEvent>::_tlsKeyMap;

template<typename LoopEvent>
Looper<LoopEvent>::Looper(ThreadCategory cate, std::shared_ptr<Loop> tsk, int64_t updateMs) :
    _category(cate), _loop(tsk), _intervalMs(updateMs)
{}

template<typename LoopEvent>
Looper<LoopEvent>::~Looper()
{

    if (!_isStopped)
    {
        std::cerr << "Destroy Looper and force close inner thread" << std::endl;
    }
    syncStop();

    if (_tid) 
    {
        delete _tid;
        _tid = nullptr;
    }
}



template<typename LoopEvent>
void Looper<LoopEvent>::init()
{
    assert(!_initialized);
    _uvLoop = ThreadLoop::getThreadLoop();
    uv_async_init(_uvLoop, &_uvAsync, &async_handle<LoopEvent>);
    _task = std::make_shared<LoopRunable>(_uvLoop, _loop, milliseconds(_intervalMs));
    Looper::setLocalData("___thread", this);
    _initialized = true;
}

template<typename LoopEvent>
void Looper<LoopEvent>::on(const std::string &name, Looper::EventCF callback)
{
    _callbackMap.add(name, callback);
}

template<typename LoopEvent>
void Looper<LoopEvent>::off(const std::string &name)
{
    _callbackMap.clear(name);
}

template<typename LoopEvent>
void Looper<LoopEvent>::emit(const std::string &name, LoopEvent &event)
{
    assert(_initialized);
    _pendingEvents.pushBack(SeqItem<LoopEvent>(genSeq(), name, event));
    notify();
}

template<typename LoopEvent>
bool Looper<LoopEvent>::isCurrentThread() const
{
    assert(_initialized);
    return _uvLoop && _uvLoop == ThreadLoop::getThreadLoop();
}


template<typename LoopEvent>
Looper<LoopEvent> * Looper<LoopEvent>::getCurrentThread()
{
    return (Looper<LoopEvent> *)Looper<LoopEvent>::getLocalData("___thread");
}

template<typename LoopEvent>
void Looper<LoopEvent>::run()
{
    assert(!_tid);
    assert(!_initialized); //call Looper<T>#init() before this

    std::condition_variable cv;
    std::mutex mtx;

    std::shared_ptr<Looper<LoopEvent> > self = shared_from_this();

    _tid = new std::thread([self, &cv]() {
        self->init();
        cv.notify_all();
        self->onRun();
    });

    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock);
}

template<typename LoopEvent>
void Looper<LoopEvent>::onRun()
{
    if (_isStopped) return;
    assert(_initialized);
    auto *tsk = _task.get();
    assert(tsk);
    Finalizer defer([tsk]() {
        tsk->afterRun();
    });
    tsk->beforeRun();
    
    //handle events before thread start
    if (_isStopped) return;
    onNotify();
    if (_isStopped) return;
    tsk->run();
}

template<typename LoopEvent>
void Looper<LoopEvent>::asyncStop()
{
    _forceStoped = true;
    notify();
}

template<typename LoopEvent>
bool Looper<LoopEvent>::syncStop()
{
    
    if (!_initialized) return false;
    if (_isStopped) return true;

    uv_loop_t *loopPtr = this->_uvLoop;
    bool *stpPtr = &_isStopped;
    wait([stpPtr, loopPtr, this]() {
        //as onStop()
        this->onNotify();
        *stpPtr = true;
        uv_stop(loopPtr);
    });
    //flush pending task list
    return true;
}

template<typename LoopEvent>
void Looper<LoopEvent>::onStop()
{
    onNotify();
    _isStopped = true;
    uv_stop(_uvLoop);
}

template<typename LoopEvent>
void Looper<LoopEvent>::join()
{
    assert(_initialized);
    assert(_tid->joinable());
    _tid->join();
}

template<typename LoopEvent>
void Looper<LoopEvent>::detach()
{
    assert(_initialized);
    _tid->detach();
}

template<typename LoopEvent>
void Looper<LoopEvent>::dispatch(Looper::DispatchF fn)
{
    _pendingFns.pushBack(SeqItem<DispatchF>(genSeq(), fn));
    if(!isCurrentThread())
    {
        notify();
    }
    else 
    {
        onNotify();
    }
}

template<typename LoopEvent> 
void Looper<LoopEvent>::wait(Looper::DispatchF fn, int timeoutMS)
{
    if (isCurrentThread()) 
    {
        _pendingFns.pushBack(SeqItem<DispatchF>(genSeq(), fn));
        onNotify();
    }
    else 
    {
        std::condition_variable cv;
        std::mutex mtx;
        std::unique_lock<std::mutex> lock(mtx);
        _pendingFns.pushBack(SeqItem<DispatchF>(genSeq(), [&cv, &mtx, &fn]() {
            std::unique_lock<std::mutex> lock2(mtx);
            fn();
            cv.notify_one();
        }));
        notify();
        if (timeoutMS > 0)
        {
            cv.wait_for(lock, milliseconds(timeoutMS));
        }
        else
        {
            cv.wait(lock);
        }
    }
}

template<typename LoopEvent>
void Looper<LoopEvent>::notify()
{
    uv_async_send(&_uvAsync);
}

template<typename LoopEvent>
void Looper<LoopEvent>::onNotify() {
    assert(isCurrentThread());

    if(_isStopped) return;

    while (_pendingEvents.size() > 0 && _pendingFns.size() > 0)
    {
        auto ev = _pendingEvents.front();
        auto fn = _pendingFns.front();
        if (ev.id < fn.id) {
            SeqItem<LoopEvent> item = _pendingEvents.popFront();
            handleEvent(item.name, item.data);
        }
        else {
            auto fn = _pendingFns.popFront();
            handleFn(fn.data);
        }
    }

    while (_pendingEvents.size() > 0)
    {
        SeqItem<LoopEvent> item = _pendingEvents.popFront();
        handleEvent(item.name, item.data);
    }
    while (_pendingFns.size() > 0)
    {
        auto fn = _pendingFns.popFront();
        handleFn(fn.data);
    }
    if (_forceStoped) {
        onStop();
    }
}

template<typename LoopEvent>
void Looper<LoopEvent>::setLocalData(const std::string &name, void *data)
{
    uv_key_t &key = _tlsKeyMap.onceInit(name, [](uv_key_t &k) {
        uv_key_create(&k);
    });
    uv_key_set(&key, data);
}
template<typename LoopEvent>
void *Looper<LoopEvent>::getLocalData(const std::string &name)
{
    if (!_tlsKeyMap.exists(name))
    {
        return nullptr;
    }
    return uv_key_get(&_tlsKeyMap[name]);
}

template<typename LoopEvent>
void Looper<LoopEvent>::handleEvent(const std::string &name, LoopEvent &ev)
{
    _callbackMap.forEach(name, [&ev](EventCF &eventCb) {
        eventCb(ev);
    });
}

template<typename LoopEvent>
inline void Looper<LoopEvent>::handleFn(const Looper::DispatchF &fn)
{
    fn();
}
