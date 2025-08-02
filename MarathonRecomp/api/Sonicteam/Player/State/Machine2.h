#pragma once

#include <Marathon.inl>
#include <Sonicteam/Player/State/IContext.h>
#include <Sonicteam/Player/State/IMachine.h>
#include <Sonicteam/SoX/AI/StateMachine.h>
#include <Sonicteam/Player/State/IContext.h>

namespace Sonicteam::Player::State
{
    class Machine2 : public SoX::AI::StateMachine<IContext>, public IMachine
    {
    public:
        MARATHON_INSERT_PADDING(0x38);

        inline SoX::AI::StateMachine<IContext>* GetBase()
        {
            return (SoX::AI::StateMachine<IContext>*)((uint8_t*)this - 0x20);
        }
    };
}
