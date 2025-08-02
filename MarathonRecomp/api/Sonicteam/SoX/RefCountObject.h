#pragma once

#include <Marathon.inl>

namespace Sonicteam::SoX
{
    class RefCountObject
    {
    public:
		xpointer<void> m_pVftable;
        be<uint32_t> m_RefCount;
    };
}
