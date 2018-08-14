#ifndef __FINALIZER_H__
#define __FINALIZER_H__

#include <functional>

class Finalizer {
public: 
    Finalizer(std::function<void()> cb) :cb(cb) {}
    virtual ~Finalizer(){
        cb();
    }
private:
    std::function<void()> cb;
};

#endif