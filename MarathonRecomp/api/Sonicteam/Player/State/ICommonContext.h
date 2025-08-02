#pragma once
#include <Marathon.inl>
#include <Sonicteam/Player/State/ContextSpeedAndJump.h>
#include <Sonicteam/Player/State/ICommonContextIF.h>
#include <Sonicteam/Player/State/IContext.h>

namespace Sonicteam::Player::State
{
    class ICommonContext : public IContext, public ICommonContextIF, public ContextSpeedAndJump
    {
    public:
        be<uint32_t> m_CurrentAnimation;
        be<float> m_LockInputTime;
        be<uint32_t> m_LastVelocityZ;
        be<uint32_t> m_LastVelocityY;
        be<uint32_t> m_LastLockInputTime;
        be<uint32_t> m_Input;
        MARATHON_INSERT_PADDING(0x38);
    };

}
