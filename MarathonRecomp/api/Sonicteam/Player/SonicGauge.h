#pragma once

#include <Marathon.inl>

namespace Sonicteam::Player
{
    class SonicGauge : public IGauge
    {
    public:
        be<float> m_Value;
        be<float> m_GroundedTime;
        be<uint32_t> m_Flags;
        be<uint32_t> m_GroundedFlags;
        be<float> m_Maximum;
        be<float> m_Green;
        be<float> m_Red;
        be<float> m_Blue;
        be<float> m_White;
        be<float> m_Sky;
        be<float> m_Yellow;
        be<float> m_Purple;
        be<float> m_Super;
        be<float> m_Heal;
        be<float> m_HealDelay;
    };
}
