#pragma once

#include <Marathon.inl>
#include <Sonicteam/SoX/RefCountObject.h>

namespace Sonicteam
{
    template <typename Type>
    struct LinkSoxNode
    {
        xpointer<LinkSoxNode<Type>> m_Prev;
        xpointer<LinkSoxNode<Type>> m_Next;
        xpointer<Type> m_This;
    };

    class ISceneParam : public SoX::RefCountObject
    {
    public:
        LinkSoxNode<void> m_Link;

    };
}
