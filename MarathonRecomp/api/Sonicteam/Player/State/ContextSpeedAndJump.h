#pragma once

#include <Marathon.inl>

namespace Sonicteam::Player::State
{
    class ContextSpeedAndJump 
    {
        be<float> base_speed_z;
        be<float> gimmick_speed_z;
        be<float> base_speed_y;
        be<float> gimmick_speed_y;
    };
}
