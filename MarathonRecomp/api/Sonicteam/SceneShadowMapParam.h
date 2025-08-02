#pragma once

#include <Marathon.inl>
#include <Sonicteam/ISceneParam.h>

namespace Sonicteam
{
    class SceneShadowMapParam : public ISceneParam
    {
    public:
        struct ShadowMapParam 
        {
            be<float> m_BaseSize;
            be<float> m_Float1;
            be<float> m_Float2;
            be<float> m_Float3;
            be<float> m_CenterOffsetX;
            be<float> m_CenterOffsetY;
            be<float> m_CenterOffsetZ;
            be<float> m_CenterOffsetW;

            ShadowMapParam() 
            {
                m_BaseSize = 1000.0;
                m_Float1 = 10.0;
                m_Float2 = 160000.0; //Far Distance???
                m_Float3 = 40000.0;
                m_CenterOffsetX = 0.0;
                m_CenterOffsetY = 0.0;
                m_CenterOffsetZ = 500.0;
                m_CenterOffsetW = 1.0;
            }
        };
    };
}
