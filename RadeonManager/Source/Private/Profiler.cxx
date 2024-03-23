// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <ADLX.h>
#include <IPerformanceMonitoring.h>

module RadeonManager.Profiler;

import RadeonManager.Manager;

using namespace RadeonProfiler;

adlx::IADLXPerformanceMonitoringServicesPtr g_PerformanceMonitoringServices{nullptr};
adlx::IADLXGPUListPtr g_GPUList{nullptr};
adlx::IADLXGPUPtr g_GPU{nullptr};

adlx::IADLXSystemMetricsSupportPtr g_SystemMetricsSupport{nullptr};
adlx::IADLXGPUMetricsSupportPtr g_GPUMetricsSupport{nullptr};

adlx::IADLXAllMetricsPtr g_AllMetrics{nullptr};
adlx::IADLXSystemMetricsPtr g_SystemMetrics{nullptr};
adlx::IADLXGPUMetricsPtr g_GPUMetrics{nullptr};
adlx::IADLXFPSPtr g_FPSMetrics{nullptr};

bool RadeonProfiler::IsRunning()
{
    return RadeonManager::IsRunning()
           && g_PerformanceMonitoringServices != nullptr
           && g_GPUList != nullptr
           && g_GPU != nullptr
           && g_SystemMetricsSupport != nullptr
           && g_GPUMetricsSupport != nullptr;
}

bool RadeonProfiler::IsLoaded()
{
    return RadeonManager::IsLoaded();
}

bool InitializeProfiler()
{
    if (!IsLoaded())
    {
        return false;
    }

    if (IsRunning())
    {
        return true;
    }

    RadeonManager::CheckADLXResult(RadeonManager::GetADLXSystemServices()->GetPerformanceMonitoringServices(&g_PerformanceMonitoringServices));
    RadeonManager::CheckADLXResult(RadeonManager::GetADLXSystemServices()->GetGPUs(&g_GPUList));
    RadeonManager::CheckADLXResult(g_GPUList->At(g_GPUList->Begin(), &g_GPU));

    RadeonManager::CheckADLXResult(g_PerformanceMonitoringServices->GetSupportedSystemMetrics(&g_SystemMetricsSupport));
    RadeonManager::CheckADLXResult(g_PerformanceMonitoringServices->GetSupportedGPUMetrics(g_GPU, &g_GPUMetricsSupport));

    return IsRunning();
}

bool RadeonProfiler::Start()
{
    if (!RadeonManager::Start())
    {
        return false;
    }

    return InitializeProfiler();
}

void RadeonProfiler::Stop()
{
    g_PerformanceMonitoringServices = nullptr;
    g_GPUList                       = nullptr;
    g_GPU                           = nullptr;
}

ProfileData RadeonProfiler::GetProfileData()
{
    ProfileData Result{};

    if (!IsRunning())
    {
        return Result;
    }

    RadeonManager::CheckADLXResult(g_PerformanceMonitoringServices->GetCurrentAllMetrics(&g_AllMetrics));
    RadeonManager::CheckADLXResult(g_AllMetrics->GetSystemMetrics(&g_SystemMetrics));
    RadeonManager::CheckADLXResult(g_AllMetrics->GetGPUMetrics(g_GPU, &g_GPUMetrics));
    RadeonManager::CheckADLXResult(g_AllMetrics->GetFPS(&g_FPSMetrics));

    g_AllMetrics->TimeStamp(&Result.TimeStamp);

    g_SystemMetrics->TimeStamp(&Result.CPU.TimeStamp);
    g_SystemMetrics->CPUUsage(&Result.CPU.Usage);
    g_SystemMetrics->SystemRAM(&Result.CPU.RAM);
    g_SystemMetrics->SmartShift(&Result.CPU.SmartShift);

    g_GPUMetrics->TimeStamp(&Result.GPU.TimeStamp);
    g_GPUMetrics->GPUUsage(&Result.GPU.Usage);
    g_GPUMetrics->GPUClockSpeed(&Result.GPU.ClockSpeed);
    g_GPUMetrics->GPUVRAMClockSpeed(&Result.GPU.VRAMClockSpeed);
    g_GPUMetrics->GPUTemperature(&Result.GPU.Temperature);
    g_GPUMetrics->GPUHotspotTemperature(&Result.GPU.HotspotTemperature);
    g_GPUMetrics->GPUPower(&Result.GPU.Power);
    g_GPUMetrics->GPUTotalBoardPower(&Result.GPU.TotalBoardPower);
    g_GPUMetrics->GPUFanSpeed(&Result.GPU.FanSpeed);
    g_GPUMetrics->GPUVRAM(&Result.GPU.VRAM);
    g_GPUMetrics->GPUVoltage(&Result.GPU.Voltage);
    g_GPUMetrics->GPUIntakeTemperature(&Result.GPU.IntakeTemperature);

    g_FPSMetrics->TimeStamp(&Result.FPS.TimeStamp);
    g_FPSMetrics->FPS(&Result.FPS.FPS);

    return Result;
}
