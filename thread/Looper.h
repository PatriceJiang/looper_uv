#ifndef __CC_LOOPER_OF_THREAD_H__
#define __CC_LOOPER_OF_THREAD_H__

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
class Looper : public std::enable_shared_from_this<Looper<LoopEvent> > {
public:

    Looper(ThreadCategory cate, std::shared_ptr<Loop> task, int64_t updateMs);
    virtual ~Looper();
    typedef std::function<void(LoopEvent&)> EventCF;
    typedef std::function<void()> DispatchF;

    void init();

    void run();
    void stop();
    bool ensureStop();
    void join();
    void detach();

    void emit(const char *name, LoopEvent &arg);
    void on(const char *name, EventCF callback);
    void off(const char *name);

    void dispatch(DispatchF fn);
    void wait(DispatchF fn, int timeoutMS = 0);
    void notify();
    void onNotify();

    bool isCurrentThread() const;

    static Looper *currentThread();
    static void setLocalData(const char *name, void *data);
    static void* getLocalData(const char *name);

    uv_loop_t *getLoop() { return loop };
    
private:
    void onStop();
    void onRun();
 
    enum ThreadCategory category;
    std::shared_ptr<Loop> _loop;
    std::shared_ptr<LoopRunable> task;
    static ThreadSafeMap<std::string, uv_key_t> tlsKeyMap;
    ThreadSafeMapArray<std::string, EventCF> callbackMap;
    ThreadSafeQueue<SeqItem<LoopEvent>> pendingEvents;
    ThreadSafeQueue<SeqItem<DispatchF>> pendingFns;

    std::atomic_uint64_t __event_fn_seq{ 0 };
    bool forceStoped{ false };
    bool stopped{ false };
    bool initialized{ false };

    uint64_t genSeq(){return __event_fn_seq.fetch_add(1);}
    void handleEvent(const std::string &name, LoopEvent &ev);
    void handleFn(DispatchF &fn);

    std::thread *tid{ nullptr };
    int64_t intervalMs;

public:
    uv_loop_t *loop{ nullptr };
    uv_async_t async;
    friend class LoopMgr;
};



using namespace std::chrono;

template<typename LoopEvent>
static void async_handle(uv_async_t *data)
{
    Looper<LoopEvent> *t = (Looper<LoopEvent> *)((char*)data - offsetof(Looper<LoopEvent>, async));
    t->onNotify();
}


//declare static field
template<typename LoopEvent>
ThreadSafeMap<std::string, uv_key_t> Looper<LoopEvent>::tlsKeyMap;

template<typename LoopEvent>
Looper<LoopEvent>::Looper(ThreadCategory cate, std::shared_ptr<Loop> tsk, int64_t updateMs) :
    category(cate), _loop(tsk), intervalMs(updateMs)
{}

template<typename LoopEvent>
Looper<LoopEvent>::~Looper()
{

    if (!stopped)
    {
        std::cerr << "Destroy Looper and force close inner thread" << std::endl;
    }
    ensureStop();
}



template<typename LoopEvent>
void Looper<LoopEvent>::init()
{
    assert(!initialized);
    loop = ThreadLoop::getThreadLoop();
    uv_async_init(loop, &async, &async_handle<LoopEvent>);
    task = std::make_shared<LoopRunable>(loop, _loop, milliseconds(intervalMs));
    Looper::setLocalData("___thread", this);
    initialized = true;
}

template<typename LoopEvent>
void Looper<LoopEvent>::on(const char *name, Looper::EventCF callback)
{
    callbackMap.add(name, callback);
}

template<typename LoopEvent>
void Looper<LoopEvent>::off(const char *name)
{
    callbackMap.clear(name);
}

template<typename LoopEvent>
void Looper<LoopEvent>::emit(const char *name, LoopEvent &event)
{
    assert(initialized);
    pendingEvents.safePushBack(SeqItem<LoopEvent>(genSeq(), name, event));
    notify();
}

template<typename LoopEvent>
bool Looper<LoopEvent>::isCurrentThread() const
{
    assert(initialized);
    return loop && loop == ThreadLoop::getThreadLoop();
}


template<typename LoopEvent>
Looper<LoopEvent> * Looper<LoopEvent>::currentThread()
{
    return (Looper<LoopEvent> *)Looper<LoopEvent>::getLocalData("___thread");
}

template<typename LoopEvent>
void Looper<LoopEvent>::run()
{
    assert(!tid);
    assert(!initialized); //call Looper<T>#init() before this

    std::condition_variable cv;
    std::mutex mtx;

    std::shared_ptr<Looper<LoopEvent> > self = shared_from_this();

    tid = new std::thread([self, &cv]() {
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
    if (stopped) return;
    assert(initialized);
    auto *tsk = task.get();
    assert(tsk);
    Finalizer defer([tsk]() {
        tsk->afterRun();
    });
    tsk->beforeRun();
    
    //handle events before thread start
    if (stopped) return;
    onNotify();
    if (stopped) return;
    tsk->run();
}

template<typename LoopEvent>
void Looper<LoopEvent>::stop()
{
    forceStoped = true;
    notify();
}

template<typename LoopEvent>
bool Looper<LoopEvent>::ensureStop()
{
    
    if (!initialized) return false;
    if (stopped) return true;

    uv_loop_t *loopPtr = this->loop;
    bool *stpPtr = &stopped;
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
    stopped = true;
    uv_stop(loop);
}

template<typename LoopEvent>
void Looper<LoopEvent>::join()
{
    assert(initialized);
    assert(tid->joinable());
    tid->join();
}

template<typename LoopEvent>
void Looper<LoopEvent>::detach()
{
    assert(initialized);
    tid->detach();
}

template<typename LoopEvent>
void Looper<LoopEvent>::dispatch(Looper::DispatchF fn)
{
    pendingFns.safePushBack(SeqItem<DispatchF>(genSeq(), fn));
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
        pendingFns.safePushBack(SeqItem<DispatchF>(genSeq(), fn));
        onNotify();
    }
    else 
    {
        std::condition_variable cv;
        std::mutex mtx;
        std::unique_lock<std::mutex> lock(mtx);
        pendingFns.safePushBack(SeqItem<DispatchF>(genSeq(), [&cv, &mtx, &fn]() {
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
    uv_async_send(&async);
}

template<typename LoopEvent>
void Looper<LoopEvent>::onNotify() {
    assert(isCurrentThread());

    if(stopped) return;

    while (pendingEvents.size() > 0 && pendingFns.size() > 0)
    {
        auto ev = pendingEvents.front();
        auto fn = pendingFns.front();
        if (ev.id < fn.id) {
            SeqItem<LoopEvent> item = pendingEvents.safePopFront();
            handleEvent(item.name, item.data);
        }
        else {
            auto fn = pendingFns.safePopFront();
            handleFn(fn.data);
        }
    }

    while (pendingEvents.size() > 0)
    {
        SeqItem<LoopEvent> item = pendingEvents.safePopFront();
        handleEvent(item.name, item.data);
    }
    while (pendingFns.size() > 0)
    {
        auto fn = pendingFns.safePopFront();
        handleFn(fn.data);
    }
    if (forceStoped) {
        onStop();
    }
}

template<typename LoopEvent>
void Looper<LoopEvent>::setLocalData(const char *name, void *data)
{
    uv_key_t &key = tlsKeyMap.onceInit(name, [](uv_key_t &k) {
        uv_key_create(&k);
    });
    uv_key_set(&key, data);
}
template<typename LoopEvent>
void *Looper<LoopEvent>::getLocalData(const char *name)
{
    if (!tlsKeyMap.exists(name))
    {
        return nullptr;
    }
    return uv_key_get(&tlsKeyMap[name]);
}

template<typename LoopEvent>
void Looper<LoopEvent>::handleEvent(const std::string &name, LoopEvent &ev)
{
    callbackMap.forEach(name, [&ev](EventCF &eventCb) {
        eventCb(ev);
    });
}

template<typename LoopEvent>
inline void Looper<LoopEvent>::handleFn(Looper::DispatchF &fn)
{
    fn();
}


#endif