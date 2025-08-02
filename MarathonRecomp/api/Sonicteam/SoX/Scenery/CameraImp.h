#pragma once

#include <Marathon.inl>
#include <Sonicteam/SoX/Scenery/Camera.h>
#include <stdx/string.h>


namespace Sonicteam::SoX::Scenery
{
    class CameraImp:Sonicteam::SoX::Scenery::Camera
    {
    public:
        static constexpr std::string_view c_Main = "Main";
        static constexpr std::string_view c_MainSky = "MainSky";
        static constexpr std::string_view c_MainPsi = "MainPsi";
        static constexpr std::string_view c_MainOC = "MainOC";
        static constexpr std::string_view c_MainGlare = "MainGlare";
        static constexpr std::string_view c_MainAfterPP = "MainAfterPP";
        static constexpr std::string_view c_Reflection0 = "Reflection0";
        static constexpr std::string_view c_ReflectionSky = "ReflectionSky";
        static constexpr std::string_view c_MainUser0 = "MainUser0";
        static constexpr std::string_view c_MainUser1 = "MainUser1";
        static constexpr std::string_view c_User0 = "User0";
        static constexpr std::string_view c_User1 = "User1";
        static constexpr std::string_view c_MainParticle = "MainParticle";
        static constexpr std::string_view c_Light0 = "Light0";
        static constexpr std::string_view c_Light1 = "Light1";
        static constexpr std::string_view c_Light2 = "Light2";
        static constexpr std::string_view c_Light3 = "Light3";
        static constexpr std::string_view c_RaderMap = "RaderMap";

   
        stdx::string m_CamName;
        be<float> m_FOV;
        be<float> m_Param1; // == 16.0
        be<float> m_Param2; // == 9.0
        be<float> m_Param3; // == 1.0
        be<float> m_Far; //
        MARATHON_INSERT_PADDING(0x1EC);
    };
}
