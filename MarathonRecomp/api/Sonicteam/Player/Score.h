#pragma once

#include <Marathon.inl>


namespace Sonicteam::Player
{
    class Score : public IScore, public IVariable, public IStepable
    {
    public:
        MARATHON_INSERT_PADDING(0x0C);
        xpointer<Sonicteam::Player::Object> m_pPlayer;

    };
}
