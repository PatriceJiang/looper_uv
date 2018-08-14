#include "Looper.h"

#include <vector>
#include <iostream>
#include <cstdint>

#include <thread>

#define MAX_GENERATOR_THREAD 20
#define GENERATE_COUNT 10000

typedef Looper<int64_t> SumLooper;

class ValueGenerator : public Loop {
public:

    void before() 
    {
        std::cout << "prepare loop" << std::endl;
    }

    void after() 
    {
        std::cout << "destroy loop" << std::endl;
    }

    void update(int dtms) //ticker
    {
        std::cout << "tick pass " << dtms << std::endl;
    }
};


int64_t total = 0;

int main(int argc, char **argv)
{
    auto sumLooper = std::make_shared<Looper<int64_t>>(ThreadCategory::ANY_THREAD, std::make_shared<ValueGenerator>(), 1000);
  
    sumLooper->run();

    //sumLooper->ensureStop();
    sumLooper->stop();
    
    sumLooper.reset();

    system("pause");

    return 0;
}