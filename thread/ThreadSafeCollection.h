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
    void pushBack(T&& ele) { TS_LOCK; _data.push_back(ele); }
    void pushBack(T& ele) { TS_LOCK; _data.push_back(ele);}

    void pushFront(T &ele) { TS_LOCK; _data.push_front(ele); }
    void pushFront(T &&ele) { TS_LOCK; _data.push_front(ele); }

    void popBack() { TS_LOCK; _data.pop_back();}
    T popFront() { TS_LOCK; assert(_data.size() > 0); T x = _data.front(); _data.pop_front(); return x; }

    T & front() {TS_LOCK;return _data.front();}

    T & back() {TS_LOCK;return _data.back();}

    size_t size() { return _data.size(); }

    std::recursive_mutex& getMutex() { return _mtx; }
    std::list<T> & getQueue() { return _data; }

private:
    std::recursive_mutex _mtx;
    std::list<T> _data;
};



template<typename K, typename V>
class ThreadSafeMap {
public:
    V & operator[](const K &key) {TS_LOCK;return _data[key];}
    void insert(const K &key, V &value){TS_LOCK;_data.insert(std::make_pair(key, value));}
    void erase(const K &key) {TS_LOCK;_data.erase(key);}
    bool exists(const K &key) { TS_LOCK; return _data.find(key) != _data.end(); }
    V getOrBuild(const K &key, std::function<V(void)> crtFn ) 
    {
        TS_LOCK;
        if (_data.find(key) == _data.end()) { //not found
            _data.insert(std::make_pair(key, crtFn()));
        }
        return _data[key];
    }
    V &onceInit(const K &key, std::function<void(V&)> initFn)
    {
        TS_LOCK;
        if (_data.find(key) == _data.end()) { //do init once
            initFn(_data[key]);
        }
        return _data[key];
    }

    std::recursive_mutex& getMutex() { return _mtx; }
private:
    std::unordered_map<K, V> _data;
    std::recursive_mutex _mtx;
};

template<typename K, typename V> 
class ThreadSafeMapArray {
public:
    void add(const K&key, V &value) { TS_LOCK; _data[key].push_back(value); }
    void clear(const K &key) { TS_LOCK; _data[key].clear(); }
    std::vector<V>& get(const K&key) { TS_LOCK; return _data[key]; }
    void forEach(const K &key, std::function<void(V&)> iterFn)
    {
        TS_LOCK;
        auto &list = _data[key];
        for (auto m = list.begin(); m != list.end(); m++)
        {
            iterFn(*m);
        }
    }

    std::recursive_mutex& getMutex() { return _mtx; }
private:
    std::unordered_map<K, std::vector<V> > _data;
    //std::mutex mtx;
    std::recursive_mutex _mtx;
};

#undef TS_LOCK

#endif