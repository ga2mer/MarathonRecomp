#pragma once
#include <Marathon.inl>
#include <Sonicteam/Player/State/CommonContext.h>
#include <Sonicteam/Player/SonicGauge.h>
#include <boost/smart_ptr/shared_ptr.h>


namespace Sonicteam::Player::State
{
    class SonicContext : public CommonContext
    {
    public:
        //Sprite
        enum GemsS:uint32_t {
            SGreen = 1,
            SRed,
            SBlue,
            SWhite,
            SSky,
            SYellow,
            SPurple,
            SSuper
        };
        enum Gems:uint32_t {
            Blue = 1,
            Red,
            Green,
            Purple,
            Sky,
            White,
            Yellow,
            Super
        };
        be<GemsS> m_CurrentGemSprite;
        boost::shared_ptr<Sonicteam::Player::SonicGauge> m_Gauge;
        uint8_t m_HomingLockOn;
        uint8_t m_DisablePlayerMovement;
        uint8_t m_AntigravityHitBox;
        uint8_t m_23F;
        uint8_t m_BoundAttackHitBox;
        uint8_t m_241;
        uint8_t m_Shrink;
        uint8_t m_ThunderGuard;
        uint8_t m_Tornado;
        uint8_t m_FPS;
        uint8_t m_ThrowGem;
        uint8_t m_SlowTime;
        uint8_t m_MachAura;
        uint8_t m_GemsEnabled;
        uint8_t m_24A;
        uint8_t m_24B;
        be<uint32_t> m_HomingFlip;
        be<Gems> m_CurrentGem;
        MARATHON_INSERT_PADDING(0x58);
    };
}
