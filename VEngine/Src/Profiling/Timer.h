/*
    Header Storing a Timer class meant to just see how long it takes to draw a proper conclusion of its efficiency.
*/

#include <chrono>

namespace VEngine
{
    class DebugTimer
    {
    public:
        DebugTimer(const char *Name)
            : _Name(Name)
        {
            _Start = std::chrono::high_resolution_clock::now();
        }

        ~DebugTimer()
        {
            _End = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(_End - _Start) ;
            VENGINE_CORE_PRINTLN(_Name << " took: " << duration)
        }

    private:
        std::chrono::_V2::system_clock::time_point _Start, _End;
        std::string _Name;
    };
}

#if VENGINE_DEBUG == VENGINE_TRUE
#define VENGINE_DEBUG_TIMER(Name) VEngine::DebugTimer _DebugTimer(Name);
#elif VENGINE_DEBUG == VENGINE_FALSE
#define VENGINE_DEBUG_TIMER(Name) ;
#endif