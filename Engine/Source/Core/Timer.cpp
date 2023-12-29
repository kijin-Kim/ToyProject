#include "EnginePCH.h"
#include "Timer.h"

namespace Engine
{
    void Timer::Tick()
    {
        static const uint64_t tickPerSeconds = glfwGetTimerFrequency();
        const uint64_t currentTick = glfwGetTimerValue();
        m_DeltaTick = std::min(currentTick - m_LastTick, tickPerSeconds / 10);
        m_TotalTick += m_DeltaTick;
        m_LastTick = currentTick;

        m_FrameCount++;
        m_SecondTimerTick += m_DeltaTick;
        if (m_SecondTimerTick >= tickPerSeconds)
        {
            m_FramesPerSecond = m_FrameCount;
            m_FrameCount = 0;
            m_SecondTimerTick %= tickPerSeconds;
        }
    }

    double Timer::GetDeltaSeconds() const
    {
        static const uint64_t tickPerSeconds = glfwGetTimerFrequency();
        return static_cast<double>(m_DeltaTick) / static_cast<double>(tickPerSeconds);
    }

    double Timer::GetTotalSeconds() const
    {
        static const uint64_t tickPerSeconds = glfwGetTimerFrequency();
        return static_cast<double>(m_TotalTick) / static_cast<double>(tickPerSeconds);
    }

    uint64_t Timer::GetFramesPerSecond() const
    {
        return m_FramesPerSecond;
    }
}
