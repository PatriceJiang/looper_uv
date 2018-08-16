#pragma once

namespace cocos2d
{
    namespace loop
    {

        class Loop {
        public:
            virtual void before() {}
            virtual void update(int dtMS) = 0;
            virtual void after() {}
        };

    }
}

