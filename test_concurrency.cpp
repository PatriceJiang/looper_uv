#include "Looper.h"

#include <vector>
#include <iostream>
#include <cstdint>

#include <thread>

#define MAX_GENERATOR_THREAD 20
#define GENERATE_COUNT 10000

using namespace cocos2d::loop;

int64_t total = 0;  // no lock required

using namespace cocos2d;
using namespace cocos2d::loop;

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
      //  std::cout << "ticker inspect total " << total << std::endl;
    }
};


int main(int argc, char **argv)
{
    ValueGenerator v;
    auto sumLooper = std::make_shared<Looper<int64_t>>(ThreadCategory::ANY_THREAD, &v, 1000);
    sumLooper->on("add", [](int64_t &v) {
        total += v;
    });
    sumLooper->run();

    std::vector<std::thread *> generators;
    for (int i = 0; i < MAX_GENERATOR_THREAD; i++) {
        auto *t = new std::thread([sumLooper, i]() {
            int64_t step = 1;
            for (int i = 0; i < GENERATE_COUNT; i++) 
            {
                sumLooper->emit("add", step); //1
            }
            sumLooper->dispatch([i]() {
                std::cout << "thread " << i << " finish generation" << std::endl;
            });
        });
        generators.push_back(t);
    }


    for (int i = 0; i < MAX_GENERATOR_THREAD; i++) {
        generators[i]->join();
    }

    sumLooper->wait([]() {
        std::cout << "total value is " << total << ", expect " << (MAX_GENERATOR_THREAD * GENERATE_COUNT) << std::endl;
    });

    sumLooper->syncStop();

    
    system("pause");

    return 0;
}