#ifndef __CC_LOOP_H__
#define __CC_LOOP_H__

class Loop {
public:
    virtual void before() {}
    virtual void update(int dtMS) = 0;
    virtual void after() {}
};

#endif