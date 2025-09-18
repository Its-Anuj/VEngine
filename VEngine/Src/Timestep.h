#pragma once

namespace VEngine
{
    struct TimeStep
    {
    public:
        TimeStep(double ts) : _ts(ts) {}

        double GetSecond() const { return _ts; }
        double GetMilliSecond() const { return _ts * 1000; }
        double GetFPS() const { return 1.0f / _ts; }

    private:
        double _ts;
    };
} // namespace VEngine
