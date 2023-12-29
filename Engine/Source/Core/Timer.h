#pragma once

namespace Engine
{
    class Timer
    {
    public:
        void Tick();
        double GetDeltaSeconds() const;
        double GetTotalSeconds() const;
        uint64_t GetFramesPerSecond() const;
        
    private:
        uint64_t m_DeltaTick = 0;
        uint64_t m_TotalTick = 0;
        uint64_t m_LastTick = 0;
        uint64_t m_SecondTimerTick = 0;
        uint64_t m_FrameCount = 0;
        uint64_t m_FramesPerSecond = 0;
    };
}
