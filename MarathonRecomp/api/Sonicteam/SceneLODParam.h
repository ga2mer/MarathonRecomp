#pragma once

#include <Marathon.inl>
#include <Sonicteam/ISceneParam.h>

namespace Sonicteam
{


    class SceneLODParam : public ISceneParam
    {
        public:
        struct LODParam 
        {
            uint8_t m_IsSet;
            MARATHON_INSERT_PADDING(3);
            be<float> m_FarDistance;
            be<float> m_ClipDistance;
            be<float> m_TerrainClipDistance;
        };
        enum LODEnum 
        {
            Main,
            MainXp,
            MainSky,
            MainPsi,
            MainOC,
            MainGlare,
            MainAfterPP,
            Light0,
            Light1,
            Light2,
            Light3,
            Reflection0,
            ReflectionSky,
            MainUser0,
            MainUser1,
            User0,
            User1,
            MainParticle,
            RaderMap
        };
        LODParam m_LODParam[19];

    };
}
