#pragma once

#include <functional>

class Finalizer {
public: 
    Finalizer(std::function<void()> cb) :_cb(cb) {}
    virtual ~Finalizer(){
        _cb();
    }
private:
    std::function<void()> _cb;
};
