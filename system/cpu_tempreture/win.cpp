#include <Windows.h>
#include <string>
#include <thread>
#include <iostream>
#include <functional>
#include <vector>

#pragma comment(lib, "Advapi32.lib")


#define IA32_THERM_STATUS_MSR  0x019C
#define IA32_TEMPERATURE_TARGET 0x01A2

#define OLS_TYPE 40000

#define IOCTL_OLS_READ_MSR \
            CTL_CODE(OLS_TYPE, 0x821, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define WINRING0_SERVICE_NAME "WinRing0Svc"

#define DRIVER_ID L"\\\\.\\WinRing0_1_2_0"

std::wstring WinRing0DriverFile() {
    if (sizeof(void*) == 8) {
        return L"WinRing0x64.sys";
    } else {
        return L"WinRing0.sys";
    }
}


struct AutoGuard
{
    AutoGuard(const std::function<void()>& func) : func_{func}{}
    ~AutoGuard() { if (func_) func_(); }
private:
    std::function<void()> func_;
};


bool startService(SC_HANDLE service_handle)
{
    BOOL success = ::StartServiceW(service_handle, 0, NULL);
    ::CloseServiceHandle(service_handle);
    auto lastError = GetLastError();
    if (success != TRUE && lastError != ERROR_ALREADY_EXISTS && lastError != ERROR_SERVICE_ALREADY_RUNNING) {
        std::cout << "StartService '" WINRING0_SERVICE_NAME "' failed with " << lastError << std::endl;
        return false;
    }
    else {
        return true;
    }
}

bool createAndStartService(const std::wstring& bin_path) {
    std::wstring service_name = L"" WINRING0_SERVICE_NAME;
    std::wstring display_name = service_name;
    auto manager_handle = ::OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!manager_handle) {
        return false;
    }
    AutoGuard guard{[manager_handle](){ ::CloseHandle(manager_handle); }};
    SC_HANDLE service_handle =
        ::CreateServiceW(
            manager_handle,
            service_name.c_str(),
            display_name.c_str(),
            SERVICE_ALL_ACCESS,
            SERVICE_KERNEL_DRIVER,
            SERVICE_DEMAND_START,
            SERVICE_ERROR_NORMAL,
            bin_path.c_str(),
            NULL,
            NULL,
            NULL,
            NULL,
            NULL);
    if (service_handle == NULL) {
        const DWORD error = GetLastError();
        if (error == ERROR_SERVICE_EXISTS) {
            service_handle = ::OpenServiceW(manager_handle, service_name.c_str(), SERVICE_ALL_ACCESS);
            ::CloseServiceHandle(manager_handle);
            if (service_handle == NULL) {
                std::cout << "Open service '" WINRING0_SERVICE_NAME "' failed with " << error << std::endl;
                return false;
            }
            return startService(service_handle);
        }
        else {
            ::CloseServiceHandle(manager_handle);
            std::cout << "Create service '" WINRING0_SERVICE_NAME "' failed with " << error << std::endl;
            return false;
        }
    }
    std::cout << "Create service '" WINRING0_SERVICE_NAME "' success" << std::endl;
    ::CloseServiceHandle(manager_handle);
    return startService(service_handle);

}

HANDLE openWinRing0Driver()
{
    auto driverPaht = L"D:/" + WinRing0DriverFile();
    if (!createAndStartService(driverPaht)) {
        return nullptr;
    }
    HANDLE handle = ::CreateFileW(DRIVER_ID, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        std::cout << "Open driver file '" DRIVER_ID "' failed with " << GetLastError() << std::endl;
    }
    return handle;
}

bool readMsr(HANDLE fileHandle, DWORD index, DWORD& eax, DWORD& edx)
{
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }
    DWORD   returnedLength = 0;
    BOOL    result = FALSE;
    BYTE    outBuf[8] = { 0 };
    result = DeviceIoControl(
        fileHandle,
        IOCTL_OLS_READ_MSR,
        &index,
        sizeof(index),
        &outBuf,
        sizeof(outBuf),
        &returnedLength,
        NULL
        );
    if (result)
    {
        memcpy(&eax, outBuf, 4);
        memcpy(&edx, outBuf + 4, 4);
    }
    return result == TRUE;
}

float getTjmaxFromMsr(HANDLE handle)
{
    static float tjmax = -1.0f;
    if (tjmax > 0) {
        return tjmax;
    }
    DWORD eax = 0;
    DWORD edx = 0;
    if (readMsr(handle, IA32_TEMPERATURE_TARGET, eax, edx)) {
        tjmax = static_cast<float>((eax >> 16) & 0xFF);
    }
    else {
        tjmax = 100.f;
    }
    return tjmax;
}

int getCpuTemperatureRandom()
{
    HANDLE handle = openWinRing0Driver();
    if (handle == nullptr) {
        return 0;
    }
    AutoGuard guard{[handle](){ ::CloseHandle(handle); }};
    DWORD eax = 0;
    DWORD edx = 0;
    if (readMsr(handle, IA32_THERM_STATUS_MSR, eax, edx) && (eax & 0x80000000) != 0) {
        auto deltaT = (eax & 0x007F0000) >> 16;
        float tjMax = getTjmaxFromMsr(handle);
        float tSlope = 1.0f;
        return tjMax - (tSlope * deltaT);
    } else {
        return 0;
    }
}

std::vector<int> getCpuTemperatureAll()
{
    HANDLE handle = openWinRing0Driver();
    if (handle == nullptr) {
        return {};
    }
    AutoGuard guard{[handle](){ ::CloseHandle(handle); }};
    DWORD eax = 0;
    DWORD edx = 0;
    std::vector<int> result;
    uint32_t cores = std::thread::hardware_concurrency();
    for (uint32_t i = 0; i < cores; i++) {
        auto old_value = SetThreadAffinityMask(GetCurrentThread(), 1 << i);
        if (readMsr(handle, IA32_THERM_STATUS_MSR, eax, edx) && (eax & 0x80000000) != 0) {
            auto deltaT = (eax & 0x007F0000) >> 16;
            float tjMax = getTjmaxFromMsr(handle);
            float tSlope = 1.0f;
            int temperature = tjMax - (tSlope * deltaT);
            result.push_back(temperature);
        }
        SetThreadAffinityMask(GetCurrentThread(), old_value);
    }
    return result;
}

int getCpuTemperatureAvg()
{
    std::vector<int> temperatures = getCpuTemperatureAll();
    int total = 0;
    for (auto value : temperatures) {
        total += value;
    }
    return total / temperatures.size();
}

int main()
{
    std::vector<int> temps = getCpuTemperatureAll();
    for (auto t : temps) {
        std::cout << t << ", ";
    }
    std::cout << std::endl;
    return 0;
}