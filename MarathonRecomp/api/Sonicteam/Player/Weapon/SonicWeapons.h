#pragma once
#include <Marathon.inl>
#include <Sonicteam/Player/IPlugIn.h>
#include <Sonicteam/Player/IFlagCommunicator.h>
#include <Sonicteam/Player/IStepable.h>
#include <Sonicteam/Player/IDynamicLink.h>
#include <Sonicteam/Player/IVariable.h>
#include <Sonicteam/Player/INotification.h>
#include <Sonicteam/SoX/Engine/Task.h>




namespace Sonicteam::Player::Weapon
{
    template <typename Type>
    struct LinkSoxNode
    {
        xpointer<LinkSoxNode<Type>> m_prev;
        xpointer<LinkSoxNode<Type>> m_next;
        Type* m_this;
    };

    template <typename Type>
    struct EntityContainer {
        xpointer<Type> Entity;
        LinkSoxNode<Type> m_Link;
    };
  
    class SonicWeapons : public IPlugIn, public IFlagCommunicator, public IStepable, public IDynamicLink, public IVariable, public INotification
    {
    public:
        MARATHON_INSERT_PADDING(0x4C);
        EntityContainer<Sonicteam::SoX::Engine::Task> m_GunDrive;
        MARATHON_INSERT_PADDING(0x24);
    };
}
