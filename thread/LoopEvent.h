#pragma once

#include <string>
namespace cocos2d
{
    namespace loop
    {
        class LoopEvent {
        public:

            LoopEvent(const std::string &msg) :message(msg) {}
            LoopEvent(const std::string &msg, void *data) :message(msg), data(data) {}
            LoopEvent(const LoopEvent &e) :eventName(e.eventName), message(e.message), data(e.data) {}
            LoopEvent(LoopEvent && e) : eventName(e.eventName), message(e.message), data(e.data) {
                e.data = nullptr;
            }

            void setName(const char *name) { eventName = name; }
            const std::string &getName() const { return eventName; }
        private:
            std::string eventName{ "[noname]" };
            std::string message;
            void *data{ nullptr };
        };

    }
}

