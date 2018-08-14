#include "Looper.h"

#include <vector>
#include <iostream>
#include <cstdint>
#include <chrono>

#include <thread>

#define MAX_GENERATOR_THREAD 20
#define GENERATE_COUNT 10000

using namespace std::chrono;

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
        auto now = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count();
        std::cout << now << std::endl;
    }
};


int64_t total = 0;

static void printThreadMsg(const char *message)
{
    std::cout << "[tid] "<< std::this_thread::get_id() << " " << message << std::endl;
}


int main(int argc, char **argv)
{
    auto sumLooper = std::make_shared<Looper<int64_t>>(ThreadCategory::ANY_THREAD, std::make_shared<ValueGenerator>(), 17);

    sumLooper->run();

    std::this_thread::sleep_for(milliseconds(3000));

    sumLooper->ensureStop();

    system("pause");

    return 0;
}