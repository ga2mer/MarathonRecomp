#include <api/Marathon.h>
#include <cpu/guest_stack_var.h>
#include <os/logger.h>
#include <user/config.h>
#include <app.h>
#include <stdx/vector.h>

// Sonicteam::Player::State::TailsContext::Update
PPC_FUNC_IMPL(__imp__sub_8221A7D8);
PPC_FUNC(sub_8221A7D8)
{
    if (!Config::TailsGauge)
    {
        __imp__sub_8221A7D8(ctx, base);
        return;
    }

    auto pTailsContext = (Sonicteam::Player::State::TailsContext*)(base + ctx.r3.u32);
    auto pPlayer = pTailsContext->m_pScore->m_pPlayer;

    if (auto pGauge = pPlayer->GetGauge<Sonicteam::Player::SonicGauge>())
    {
        pGauge->m_Maximum = 100.0f;
        pGauge->m_Value = (100.0f / pTailsContext->m_FlightDuration) * pTailsContext->m_FlightTime;
    }

    auto pState = pPlayer->m_spStateMachine->GetBase()->m_pState;
    auto pGameImp = App::s_pApp->m_pDoc->GetDocMode<Sonicteam::GameMode>()->m_pGameImp;

    auto maturityValue = 1.0f;

    // Set maturity value if the current state is Sonicteam::Player::State::TailsFlight.
    if (pState->m_pVftable.ptr == 0x82005404)
        maturityValue = (1.0f / pTailsContext->m_FlightLimit) * pState->m_Time;

    for (int i = 0; i < 4; i++)
    {
        if (pGameImp->m_PlayerData[i].ActorID == pPlayer->m_ActorID.get())
            pGameImp->m_PlayerData[i].MaturityValue = maturityValue;
    }

    __imp__sub_8221A7D8(ctx, base);
}


// Sonicteam::Player::State::*Context IDynamicLink::Init
PPC_FUNC_IMPL(__imp__sub_8220F330);
PPC_FUNC(sub_8220F330)
{
    if (!Config::TailsGauge)
    {
        __imp__sub_8220F330(ctx, base);
        return;
    }

    auto pDynamicLink = (Sonicteam::Player::IDynamicLink*)(base + ctx.r3.u32);
    auto spPlugin = (boost::shared_ptr<Sonicteam::Player::IPlugIn>*)(base + ctx.r4.u32);

    // If this call is from TailsContext and this plugin is Sonicteam::Player::Score, set up the action gauge.
    // This is typically set up by OpenGauge in Lua, but we can't do this here.
    if (pDynamicLink->m_pVftable.ptr == 0x8200B7F4 && spPlugin->get()->m_Name == "score")
    {
        auto pScore = (Sonicteam::Player::Score*)spPlugin->get();
        auto pSonicGauge = GuestToHostFunction<Sonicteam::Player::SonicGauge*>(sub_8223F208, g_userHeap.Alloc(sizeof(Sonicteam::Player::SonicGauge)));

        guest_stack_var<boost::shared_ptr<Sonicteam::Player::IGauge>> spSonicGauge;

        // Make shared pointer.
        GuestToHostFunction<void>(sub_821BEAB0, spSonicGauge.get(), pSonicGauge);

        // Add gauge plugin to player.
        GuestToHostFunction<void>(sub_821BECE0, pScore->m_pPlayer.get(), spSonicGauge.get(), 1);

        pScore->m_pPlayer->m_spGauge = *spSonicGauge.get();
    }

    __imp__sub_8220F330(ctx, base);
}




////////////////////////////////////////////////////////
/////////////   Sonic Gauge Restoration ////////////////
////////////////////////////////////////////////////////


const Sonicteam::Player::State::SonicContext::GemsS gemConversionTable[] = {
    Sonicteam::Player::State::SonicContext::GemsS::SBlue,   
    Sonicteam::Player::State::SonicContext::GemsS::SRed,    
    Sonicteam::Player::State::SonicContext::GemsS::SGreen,  
    Sonicteam::Player::State::SonicContext::GemsS::SPurple, 
    Sonicteam::Player::State::SonicContext::GemsS::SSky,    
    Sonicteam::Player::State::SonicContext::GemsS::SWhite,  
    Sonicteam::Player::State::SonicContext::GemsS::SYellow, 
    Sonicteam::Player::State::SonicContext::GemsS::SSuper   
};

//Check Gauge Drain
//SonicTeam::Player::SonicGauge (IVariable), IVariable::Init(REF_TYPE(SonicTeam::LuaSystem))
PPC_FUNC_IMPL(__imp__sub_82217FC0);
PPC_FUNC(sub_82217FC0) {

    if (!Config::SonicGauge)
    {
        __imp__sub_82217FC0(ctx, base);
        return;
    }

    auto context = (Sonicteam::Player::State::SonicContext*)g_memory.Translate(ctx.r3.u32);
    auto gauge = context->m_Gauge.get();

    using enum Sonicteam::Player::State::SonicContext::Gems;
    auto gem_id = (Sonicteam::Player::State::SonicContext::Gems)(ctx.r4.u32);

    switch (gem_id) {
        case Yellow:
            if (context->m_ThunderGuard) break;
        case Blue:
        case Green:
        case Sky:
        case White:
        case Super: 
        {
            size_t index = gemConversionTable[gem_id - 1] - 1;
            if (gauge->m_Value >= (&gauge->m_Green)[index].get()) {
                ctx.r3.u64 = 1;
                return;
            }
            break;
        }
        case Red:
        case Purple:
            if (context->m_24A == 0) 
            {
                ctx.r3.u64 = 1;
                return;
            }
            break;
    }
    ctx.r3.u64 = 0;
    return;

}
//Gauge Drain
PPC_FUNC_IMPL(__imp__sub_82218068);
PPC_FUNC(sub_82218068) {

    if (!Config::SonicGauge)
    {
        __imp__sub_82217FC0(ctx, base);
        return;
    }

    auto context = (Sonicteam::Player::State::SonicContext*)g_memory.Translate(ctx.r3.u32);
    auto gauge = context->m_Gauge.get();

    using enum Sonicteam::Player::State::SonicContext::Gems;
    auto gem_id = (Sonicteam::Player::State::SonicContext::Gems)(ctx.r4.u32);
    double delta = ctx.f1.f64;

    switch (gem_id) {
    case Blue:
        case Green:
        case Yellow:
        case Sky:
        case White:
        case Super:
        {
            uint32_t index = gemConversionTable[gem_id - 1] - 1;
            gauge->m_Value = gauge->m_Value.get() - (&gauge->m_Green)[index].get();
            gauge->m_GroundedTime = 0.0;
            break;
        }
        case Red:
        case Purple:
        {
            uint32_t index = gemConversionTable[gem_id - 1] - 1;
            gauge->m_Value = gauge->m_Value.get() - (&gauge->m_Green)[index].get() * delta;
            gauge->m_GroundedTime = 0.0;
            if (gauge->m_Value <= 0)
            {
                gauge->m_Value = 0.0;
                context->m_Shrink = 0;
                context->m_SlowTime = 0;
                context->m_24A = 1;

            }
            break;
        }
    }
}


PPC_FUNC_IMPL(__imp__sub_8223F360);
PPC_FUNC(sub_8223F360) {

    auto IVariable = ctx.r3.u32;
    auto RefTypeLuaSystem = ctx.r4.u32;
    __imp__sub_8223F360(ctx, base);
    if (!Config::SonicGauge)
        return;

    auto gauge = (Sonicteam::Player::SonicGauge*)g_memory.Translate(IVariable - 0x20);
    auto buffer = g_userHeap.Alloc<stdx::string>();

    // if (gauge->m_Gem == 0.0) if m_Gem not intitialized by __imp__sub_8223F360
    *buffer = "c_gauge_green";
    if (gauge->m_Green.get() == 0.0)
        gauge->m_Green = GuestToHostFunction<float>(sub_821EA350, RefTypeLuaSystem, g_memory.MapVirtual(buffer));

    *buffer = "c_gauge_red";
    if (gauge->m_Red.get()== 0.0)
        gauge->m_Red = GuestToHostFunction<float>(sub_821EA350, RefTypeLuaSystem, g_memory.MapVirtual(buffer));

    *buffer = "c_gauge_blue";
    if (gauge->m_Blue.get() == 0.0)
        gauge->m_Blue = GuestToHostFunction<float>(sub_821EA350, RefTypeLuaSystem, g_memory.MapVirtual(buffer));

    *buffer = "c_gauge_white";
    if (gauge->m_White.get() == 0.0)
        gauge->m_White = GuestToHostFunction<float>(sub_821EA350, RefTypeLuaSystem, g_memory.MapVirtual(buffer));

    *buffer = "c_gauge_sky";
    if (gauge->m_Sky.get() == 0.0)
        gauge->m_Sky = GuestToHostFunction<float>(sub_821EA350, RefTypeLuaSystem, g_memory.MapVirtual(buffer));

    *buffer = "c_gauge_yellow";
    if (gauge->m_Yellow.get() == 0.0)
        gauge->m_Yellow = GuestToHostFunction<float>(sub_821EA350, RefTypeLuaSystem, g_memory.MapVirtual(buffer));

    *buffer = "c_gauge_purple";
    if (gauge->m_Purple.get() == 0.0)
        gauge->m_Purple = GuestToHostFunction<float>(sub_821EA350, RefTypeLuaSystem, g_memory.MapVirtual(buffer));

    *buffer = "c_gauge_super";
    if (gauge->m_Super.get() == 0.0)
        gauge->m_Super = GuestToHostFunction<float>(sub_821EA350, RefTypeLuaSystem, g_memory.MapVirtual(buffer));


    buffer->~string();
    g_userHeap.Free(buffer);

    /* For Testing
    gauge->m_Green = 10;
    gauge->m_Blue = 20;
    gauge->m_White = 40;
    gauge->m_Sky = 60;
    gauge->m_Yellow = 80;
    gauge->m_Super = 100;
    gauge->m_Red = 20;
    gauge->m_Purple = 40;
    */
}
 

void SonicGaugeRestorationGaugeGemSpriteResetFix(PPCRegister& r_GameImp) {

    Sonicteam::GameImp* pGameImp = (Sonicteam::GameImp*)g_memory.Translate(r_GameImp.u32);
    for (int i = 0; i < 4; i++) pGameImp->m_PlayerData[i].GemIndex = 0;
}


void SonicGaugeRestorationGaugeFlagFix(PPCRegister& r_gauge, PPCRegister& r_context) {

    if (!Config::SonicGauge || !r_gauge.u32)
        return;



    auto pGauge = (Sonicteam::Player::SonicGauge*)g_memory.Translate(r_gauge.u32);
    auto PContext = (Sonicteam::Player::State::SonicContext*)g_memory.Translate(r_context.u32);
    if ((uint32_t)(static_cast<Sonicteam::Player::IPlugIn*>(pGauge)->m_pVftable.get()) != 0x8200D4D8) // != SonicGauge 
        return;

    auto weapons = PContext->m_pScore->m_pPlayer->GetPlugin<Sonicteam::Player::Weapon::SonicWeapons>("sonic_weapons");
    


    if (PContext->m_Tornado != 0 || PContext->m_CurrentAnimation == 0xCB || PContext->m_CurrentAnimation == 0xCC || PContext->m_CurrentAnimation == 0x46 || PContext->m_CurrentAnimation == 0xCE ||  weapons->m_GunDrive.Entity != 0)
    {
        pGauge->m_GroundedFlags = 1; // Lock game
    }
    else
    {
        using enum Sonicteam::Player::State::SonicContext::Gems;

        if ((PContext->m_Input.get() & 0x10000) != 0) {
            PContext->m_24A = 0;
        }
            
        if ((PContext->m_Input.get() & 0x20000) != 0 && (PContext->m_CurrentGem == Red || PContext->m_CurrentGem == Purple))
        {
            pGauge->m_GroundedFlags = 1; 
            if (PContext->m_24A) 
            {
                pGauge->m_GroundedFlags = 0;
                PContext->m_Shrink = 0;
                PContext->m_SlowTime = 0;
            }
        }
        else if ((PContext->m_PostureFlags.get() & Sonicteam::Player::State::CommonContext::CC_GROUND) != 0 || PContext->m_24A)
        {
            pGauge->m_GroundedFlags = 0;

        }
    }

}
//Sonic Gauge Restoration
