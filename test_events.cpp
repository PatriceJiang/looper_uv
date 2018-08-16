#include "Looper.h"

#include <vector>
#include <iostream>
#include <cstdint>

#include <thread>

#define MAX_GENERATOR_THREAD 20
#define GENERATE_COUNT 10000

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
        std::cout << "tick pass " << dtms << std::endl;
    }
};


int64_t total = 0;

static void printThreadMsg(const char *message)
{
    std::cout << "[tid] "<< std::this_thread::get_id() << " " << message << std::endl;
}


int main(int argc, char **argv)
{
    auto loop = std::make_shared<ValueGenerator>();
    auto sumLooper = std::make_shared<Looper<int64_t>>(ThreadCategory::ANY_THREAD, loop.get(), 1000);

    sumLooper->run();

    printThreadMsg("main thread");
    sumLooper->dispatch([](){
        printThreadMsg("dispatch 1");
    });

    for (int i = 0; i < 10; i++) {
        sumLooper->dispatch([i]() {
            char buff[30] = { 0 };
            snprintf(buff, 30, "dispatch %d", i + 2);
            printThreadMsg(buff);
        });
    }

    sumLooper->on("ddd", [](int64_t &msg) {
        char buff[30] = { 0 };
        snprintf(buff, 30, "event %lld", msg);
        printThreadMsg(buff);
    });

    int64_t d = 32223;
    sumLooper->emit("ddd", d);

    sumLooper->syncStop();

    system("pause");

    return 0;
}