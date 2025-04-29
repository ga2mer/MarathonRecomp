#pragma once

#include <api/Marathon.h>

struct InstallerWizard
{
    static inline bool s_isVisible = false;

    static void Init();
    static void Draw();
    static void Shutdown();
    static bool Run(std::filesystem::path installPath);
};
