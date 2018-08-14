#ifndef __THREAD_SAFE_Collection_H__
#define __THREAD_SAFE_Collection_H__

#include <list>
#include <vector>
#include <unordered_map>
#include <functional>

#include <thread>
#include <mutex>

#include <cassert>

#ifdef TS_LOCK
#error "TS_LOCK already defined!"
#endif // TS_LOCK


#define TS_LOCK std::lock_guard<std::recursive_mutex> guard(mtx)

template<typename T>
class ThreadSafeQueue {
public:
    void safePushBack(T&& ele) { TS_LOCK; data.push_back(ele); }
    void safePushBack(T& ele) { TS_LOCK; data.push_back(ele);}

    void safePushFront(T &ele) { TS_LOCK; data.push_front(ele); }
    void safePushFront(T &&ele) { TS_LOCK; data.push_front(ele); }

    void safePopBack() { TS_LOCK; data.pop_back();}
    T safePopFront() { TS_LOCK; assert(data.size() > 0); T x = data.front(); data.pop_front(); return x; }

    T & front() {TS_LOCK;return data.front();}

    T & back() {TS_LOCK;return data.back();}

    size_t size() { return data.size(); }

    std::recursive_mutex& getMutex() { return mtx; }
    std::list<T> & getQueue() { return data; }
private:
    std::recursive_mutex mtx;
    std::list<T> data;
};



template<typename K, typename V>
class ThreadSafeMap {
public:
    V & operator[](const K &key) {TS_LOCK;return data[key];}
    void insert(const K &key, V &value){TS_LOCK;data.insert(std::make_pair(key, value));}
    void erase(const K &key) {TS_LOCK;data.erase(key);}
    bool exists(const K &key) { TS_LOCK; return data.find(key) != data.end(); }
    V getOrBuild(const K &key, std::function<V(void)> crtFn ) 
    {
        TS_LOCK;
        if (data.find(key) == data.end()) { //not found
            data.insert(std::make_pair(key, crtFn()));
        }
        return data[key];
    }
    V &onceInit(const K &key, std::function<void(V&)> initFn)
    {
        TS_LOCK;
        if (data.find(key) == data.end()) { //do init once
            initFn(data[key]);
        }
        return data[key];
    }

    std::recursive_mutex& getMutex() { return mtx; }
private:
    std::unordered_map<K, V> data;
    std::recursive_mutex mtx;
};

template<typename K, typename V> 
class ThreadSafeMapArray {
public:
    void add(const K&key, V &value) { TS_LOCK; data[key].push_back(value); }
    void clear(const K &key) { TS_LOCK; data[key].clear(); }
    std::vector<V>& get(const K&key) { TS_LOCK; return data[key]; }
    void forEach(const K &key, std::function<void(V&)> iterFn)
    {
        TS_LOCK;
        auto &list = data[key];
        for (auto m = list.begin(); m != list.end(); m++)
        {
            iterFn(*m);
        }
    }

    std::recursive_mutex& getMutex() { return mtx; }
private:
    std::unordered_map<K, std::vector<V> > data;
    //std::mutex mtx;
    std::recursive_mutex mtx;
};

#undef TS_LOCK

#endif