#pragma once
#include <Marathon.inl>
#include <Sonicteam/Player/State/ICommonContext.h>
#include <Sonicteam/Player/IExportPostureRequestFlag.h>
#include <Sonicteam/Player/IExportWeaponRequestFlag.h>
#include <Sonicteam/Player/Score.h>

namespace Sonicteam::Player::State
{

  
  
    class CommonContext : public ICommonContext, public IExportPostureRequestFlag, public IExportWeaponRequestFlag
    {
    public:

        //(Gordon Ramsay)
        enum PostureEnum:uint32_t{
            CC_GROUND = 0x1,				// detects Ground 
            CC_WALLBRUSHING = 0x8,		    // detects brushing against a wall
            CC_HEADONWALL = 0x10,			// detects head-on wall collision (will always be enabled w/ 2^3)
            CC_RAILGRIND = 0x40,			// Rail grinding
            CC_BEFOREFALL = 0x100,		    // seems like Neutral or Pre-Fall? The moment before transitioning from the Jump to Fall animation
            CC_FALL = 0x200,				// FALL
            CC_WATERLCOL = 0x800,			// Water collision (making you slide down an incline)? 2^8(CC_BEFOREFALL) is often set with this
            CC_LIGHTDASHIN = 0x4000,		// Light Dashing (2 ^ 14)
            CC_QUICKROTATE = 0x8000,		// is moving the control stick in a non-forward direction (usually only active for a frame since the character instantly rotates)
            CC_TENTATIVE = 0x10000,		    // is Tentative collision 
            CC_WATERSLIDE = 0x20000,		// Water sliding (2^17)	
            CC_GRASS = 0x100000,			// grass 2^20
            CC_DIRTCLAY = 0x200000,		    // dirt/clay (2^21)
            CC_STONE = 0x400000,			// stone (2^22)
            CC_SHORELINE = 0x1000000,		// shoreline? sand? Uncertain (2^24) 
        };

        MARATHON_INSERT_PADDING(0x2C);
        be<uint32_t> m_PostureFlags;
        MARATHON_INSERT_PADDING(0x60);
        xpointer<Score> m_pScore;
        MARATHON_INSERT_PADDING(0x104);
    };
}
