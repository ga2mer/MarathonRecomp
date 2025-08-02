#pragma once

#include <Marathon.inl>

namespace Sonicteam
{
    class GameImp : public SoX::MessageReceiver
    {
    public:
        struct PlayerData
        {
            be<uint32_t> ActorID;
            be<uint32_t> RingCount;
            be<uint32_t> TownRingCount;
            be<uint32_t> LifeCount;
            be<uint32_t> ScoreCount;
            be<float> AliveTime;
            be<float> Time;
            MARATHON_INSERT_PADDING(4);
            be<float> SectionSaveTime;
            be<float> GaugeValue;
            be<uint32_t> MaturityLevel;
            be<float> MaturityValue;
            MARATHON_INSERT_PADDING(4);
            be<uint32_t> ExtendRingCount;
            be<uint32_t> GemIndex;
            MARATHON_INSERT_PADDING(0x10);
        };

        MARATHON_INSERT_PADDING(0xE3C);
        PlayerData m_PlayerData[4];
    };
}
