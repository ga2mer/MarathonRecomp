#include "app.h"
#include "kernel/memory.h"
// #include <api/Marathon.h>
#include <gpu/video.h>
#include <cstdio>
#include <install/installer.h>
#include <kernel/function.h>
#include <os/process.h>
#include <os/logger.h>
#include <patches/audio_patches.h>
#include <patches/inspire_patches.h>
#include <ui/game_window.h>
#include <user/config.h>
#include <user/paths.h>
#include <user/registry.h>

void App::Restart(std::vector<std::string> restartArgs)
{
    os::process::StartProcess(os::process::GetExecutablePath(), restartArgs, os::process::GetWorkingDirectory());
    Exit();
}

void App::Exit()
{
    Config::Save();

#ifdef _WIN32
    timeEndPeriod(1);
#endif

    std::_Exit(0);
}

// PPC_FUNC_IMPL(__imp__sub_828B2BF8);
// PPC_FUNC(sub_828B2BF8)
// {
//     LOGN_WARNING("sub_828B2BF8 call");

//     __imp__sub_828B2BF8(ctx, base);
// }

PPC_FUNC_IMPL(__imp__sub_824A7598);
PPC_FUNC(sub_824A7598)
{
    // LOGN_WARNING("sub_824A7598 RUN INTRO");

    __imp__sub_824A7598(ctx, base);
}


// // SWA::CApplication::CApplication
// PPC_FUNC_IMPL(__imp__sub_824EB490);
// PPC_FUNC(sub_824EB490)
PPC_FUNC_IMPL(__imp__sub_825B2160);
PPC_FUNC(sub_825B2160)
{
    LOGN_WARNING("Init app");
    App::s_isInit = true;
    App::s_isMissingDLC = true;
    App::s_language = Config::Language;

    // SWA::SGlobals::Init();
    Registry::Save();

    __imp__sub_825B2160(ctx, base);
}

// PPC_FUNC_IMPL(__imp__sub_828FEB90);
// PPC_FUNC(sub_828FEB90)
// {
//     LOGN_WARNING("prerender present");
//     // __builtin_trap();
//     printf("%x %x %x %x\n", ctx.r4.u32, ctx.r5.u32, ctx.r6.u32, ctx.r7.u32);

//     __imp__sub_828FEB90(ctx, base);
// }

uint32_t read_be32(uint8_t* mem, uint32_t addr) {
    return (mem[addr] << 24) | (mem[addr + 1] << 16) |
           (mem[addr + 2] << 8) | mem[addr + 3];
}

PPC_FUNC_IMPL(__imp__sub_82650058);
PPC_FUNC(sub_82650058)
{
    // __builtin_trap();
    auto _r5 = (uint8_t*)g_memory.Translate(ctx.r5.u32);

    uint32_t data_ptr = read_be32(_r5, 4);      // Pointer to char data
    uint32_t size = read_be32(_r5, 0 + 0x14);      // Length of string
    uint32_t capacity = read_be32(_r5, 0 + 0x18);  // Capacity

    // printf("Pointer: 0x%08x, Size: %u, Capacity: %u\n", data_ptr, size, capacity);

    // // Read the string data from the emulated memory
    if (data_ptr && size > 0) {
        printf("Loading file from pkg or arc: ");
        for (uint32_t i = 0; i < size; i++) {
            printf("%c", base[data_ptr + i]);
        }
        printf("\n");
    }

    __imp__sub_82650058(ctx, base);
}

PPC_FUNC_IMPL(__imp__sub_825867A8);
PPC_FUNC(sub_825867A8)
{
    auto _r3 = (char*)g_memory.Translate(ctx.r4.u32);
    printf("CreateThread by name: %s\n", _r3);

    __imp__sub_825867A8(ctx, base);
}

// PPC_FUNC_IMPL(__imp__sub_825B1A28);
// PPC_FUNC(sub_825B1A28)
// {
//     LOGN_WARNING("SWAP");
//     // __builtin_trap();
//     // printf("%x \n", ctx.r3.u32);

//     __imp__sub_825B1A28(ctx, base);
// }


PPC_FUNC_IMPL(__imp__sub_82744840);
PPC_FUNC(sub_82744840)
{
    LOGN_WARNING("RenderFrame");

    __imp__sub_82744840(ctx, base);
}

// PPC_FUNC_IMPL(__imp__sub_82558CD0);
// PPC_FUNC(sub_82558CD0)
// {
//     LOGN_WARNING("D3DDevice_Present");

//     __builtin_trap();

//     __imp__sub_82558CD0(ctx, base);
// }

PPC_FUNC_IMPL(__imp__sub_82538B48);
PPC_FUNC(sub_82538B48)
{
    // LOGN_WARNING("NtWaitForSingleObjectEx timeout converter");
    // printf("timeout in ms %d\n", ctx.r4.u32);
    if (ctx.r4.u32 == 0x1) {
        // __builtin_trap();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        ctx.r4.u32 = -1;
    }

    __imp__sub_82538B48(ctx, base);
}

// PPC_FUNC_IMPL(__imp__sub_825B1A28);
// PPC_FUNC(sub_825B1A28)
// {
//     LOGN_WARNING("MaybePresent");
//     std::thread::id tid = std::this_thread::get_id();
//     // size_t hash_val = std::hash<std::thread::id>{}(tid);
//     printf("present thread id: %x\n", XXH32(&tid, sizeof(tid), 0));
//     __builtin_trap();

//     __imp__sub_825B1A28(ctx, base);
// }

// PPC_FUNC_IMPL(__imp__sub_825E7AD8);
// PPC_FUNC(sub_825E7AD8)
// {
//     LOGN_WARNING("Sonicteam::Heap::HeapAllocCustom");
//     // printf("%x \n", ctx.r3.u32);

//     __imp__sub_825E7AD8(ctx, base);
// }

// PPC_FUNC_IMPL(__imp__sub_825E6F70);
// PPC_FUNC(sub_825E6F70)
// {
//     LOGN_WARNING("Sonicteam::SpanverseMemory::AllocMemory");
//     printf("size: %x\n", ctx.r3.u32);
//     auto initialized = *reinterpret_cast<be<int>*>(g_memory.Translate(0x82D3BAA8));
//     printf("%x %x\n", initialized, &initialized);

//     __imp__sub_825E6F70(ctx, base);
// }
// 82186158
// PPC_FUNC_IMPL(__imp__sub_825E7918);
PPC_FUNC(sub_825E7918)
{
    LOGN_WARNING("spanverse allocex");
    // printf("%x\n", ctx.r3.u32);

    sub_82186158(ctx, base);
}

PPC_FUNC_IMPL(__imp__sub_825822D0);
PPC_FUNC(sub_825822D0)
{
    LOGN_WARNING("sub_825822D0");

    __imp__sub_825822D0(ctx, base);
}

static std::thread::id g_mainThreadId = std::this_thread::get_id();

// // SWA::CApplication::Update
PPC_FUNC_IMPL(__imp__sub_825EA610);
PPC_FUNC(sub_825EA610)
{
    __imp__sub_825EA610(ctx, base);
    // LOGN_WARNING("::Update");
    Video::WaitOnSwapChain();

    // Correct small delta time errors.
    if (Config::FPS >= FPS_MIN && Config::FPS < FPS_MAX)
    {
        double targetDeltaTime = 1.0 / Config::FPS;

        if (abs(ctx.f1.f64 - targetDeltaTime) < 0.00001)
            ctx.f1.f64 = targetDeltaTime;
    }

    App::s_deltaTime = ctx.f1.f64;

    // printf("s_deltaTime = %f\n", ctx.f1.f64);
    App::s_time += App::s_deltaTime;

    // This function can also be called by the loading thread,
    // which SDL does not like. To prevent the OS from thinking
    // the process is unresponsive, we will flush while waiting
    // for the pipelines to finish compiling in video.cpp.
    if (std::this_thread::get_id() == g_mainThreadId)
    {
        SDL_PumpEvents();
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
        GameWindow::Update();
    }

//     AudioPatches::Update(App::s_deltaTime);
//     InspirePatches::Update();

//     // Apply subtitles option.
//     if (auto pApplicationDocument = SWA::CApplicationDocument::GetInstance())
//         pApplicationDocument->m_InspireSubtitles = Config::Subtitles;

//     if (Config::EnableEventCollisionDebugView)
//         *SWA::SGlobals::ms_IsTriggerRender = true;

//     if (Config::EnableGIMipLevelDebugView)
//         *SWA::SGlobals::ms_VisualizeLoadedLevel = true;

//     if (Config::EnableObjectCollisionDebugView)
//         *SWA::SGlobals::ms_IsObjectCollisionRender = true;

//     if (Config::EnableStageCollisionDebugView)
//         *SWA::SGlobals::ms_IsCollisionRender = true;

    // __imp__sub_825EA610(ctx, base);
}


void DebugZlibMidAsmHook(PPCRegister& id)
{
    // printf("DebugZlibMidAsmHook: %x\n", id.u64);
}

void DebugZlibMidAsm2Hook(PPCRegister& id)
{
    if (id.u64 == 10) {
        printf("DebugZlibMidAsm2Hook: %d\n", id.u64);
    }
}

void DebugIndirectCall(PPCRegister& id)
{
    printf("DebugIndirectCall: %x\n", id.u64);
}

