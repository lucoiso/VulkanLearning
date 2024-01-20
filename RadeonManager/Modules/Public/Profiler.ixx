// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include "RadeonManagerModule.hpp"
#include <cstdint>

export module RadeonManager.Profiler;

export namespace RadeonProfiler
{
    struct RADEONMANAGERMODULE_API CPUProfileData
    {
        std::int64_t TimeStamp{0};
        double Usage{0.0};
        std::int32_t RAM{0};
        std::int32_t SmartShift{0};
    };

    struct RADEONMANAGERMODULE_API GPUProfileData
    {
        std::int64_t TimeStamp{0};
        double Usage{0.0};
        std::int32_t ClockSpeed{0};
        std::int32_t VRAMClockSpeed{0};
        double Temperature{0.0};
        double HotspotTemperature{0.0};
        double Power{0.0};
        double TotalBoardPower{0.0};
        std::int32_t FanSpeed{0};
        std::int32_t VRAM{0};
        std::int32_t Voltage{0};
        double IntakeTemperature{0.0};
    };

    struct RADEONMANAGERMODULE_API FPSProfileData
    {
        std::int64_t TimeStamp{0};
        std::int32_t FPS{0};
    };

    struct RADEONMANAGERMODULE_API ProfileData
    {
        std::int64_t TimeStamp{0};
        CPUProfileData CPU{};
        GPUProfileData GPU{};
        FPSProfileData FPS{};
    };

    [[nodiscard]] bool IsRunning();

    [[nodiscard]] bool IsLoaded();

    [[nodiscard]] bool Start();

    void Stop();

    [[nodiscard]] ProfileData GetProfileData();
}// namespace RadeonProfiler
