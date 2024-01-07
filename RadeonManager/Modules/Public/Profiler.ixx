// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include "RadeonManagerModule.hpp"
#include <cstdint>

export module RadeonManager.Profiler;

export namespace RadeonProfiler
{
    struct RADEONMANAGERMODULE_H CPUProfileData
    {
        std::int64_t TimeStamp {0};
        double Usage {0.0};
        std::int32_t RAM {0};
        std::int32_t SmartShift {0};
    };

    struct RADEONMANAGERMODULE_H GPUProfileData
    {
        std::int64_t TimeStamp {0};
        double Usage {0.0};
        std::int32_t ClockSpeed {0};
        std::int32_t VRAMClockSpeed {0};
        double Temperature {0.0};
        double HotspotTemperature {0.0};
        double Power {0.0};
        double TotalBoardPower {0.0};
        std::int32_t FanSpeed {0};
        std::int32_t VRAM {0};
        std::int32_t Voltage {0};
        double IntakeTemperature {0.0};
    };

    struct RADEONMANAGERMODULE_H FPSProfileData
    {
        std::int64_t TimeStamp {0};
        std::int32_t FPS {0};
    };

    struct RADEONMANAGERMODULE_H ProfileData
    {
        std::int64_t TimeStamp {0};
        CPUProfileData CPU {};
        GPUProfileData GPU {};
        FPSProfileData FPS {};
    };

    [[nodiscard]] bool RADEONMANAGERMODULE_H IsRunning();
    [[nodiscard]] bool RADEONMANAGERMODULE_H IsLoaded();

    [[nodiscard]] bool RADEONMANAGERMODULE_H Start();
    void RADEONMANAGERMODULE_H Stop();

    [[nodiscard]] ProfileData RADEONMANAGERMODULE_H GetProfileData();
}// namespace RadeonProfiler
