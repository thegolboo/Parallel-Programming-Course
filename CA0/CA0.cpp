#include <iostream>
#include <intrin.h> // For __cpuid and __cpuidex
#include <vector>

void getCPUInfo(int leaf, int subleaf, std::vector<int>& info) {
    __cpuidex(info.data(), leaf, subleaf);
}

int main() {
    std::vector<int> info(4);

    // Get maximum CPUID leaf (leaf 0)
    getCPUInfo(0, 0, info);
    int maxCPUIDLeaf = info[0];
    std::cout << "Maximum CPUID leaf: " << maxCPUIDLeaf << std::endl;

    // Get processor brand string (leaf 0x80000002 - 0x80000004)
    char brand[49] = { 0 };
    getCPUInfo(0x80000002, 0, info);
    memcpy(brand, info.data(), sizeof(info));
    getCPUInfo(0x80000003, 0, info);
    memcpy(brand + 16, info.data(), sizeof(info));
    getCPUInfo(0x80000004, 0, info);
    memcpy(brand + 32, info.data(), sizeof(info));
    std::cout << "CPU Brand: " << brand << std::endl;

    // Get CPU model, family, stepping (leaf 1)
    getCPUInfo(1, 0, info);
    int model = (info[0] >> 4) & 0xF;
    int family = (info[0] >> 8) & 0xF;
    int stepping = info[0] & 0xF;
    std::cout << "CPU Model: " << model << ", Family: " << family << ", Stepping: " << stepping << std::endl;

    // Check for hyperthreading support (EBX bit 28 in leaf 1)
    bool hyperThreading = info[3] & (1 << 28);
    std::cout << "Hyper-Threading: " << (hyperThreading ? "Supported" : "Not Supported") << std::endl;

    // Use CPUID leaf 0xB for extended topology enumeration to determine core and logical processor count
    int totalLogicalProcessors = 0;
    int logicalProcessorsPerCore = 0;

    int subleaf = 0;
    while (true) {
        getCPUInfo(0xB, subleaf, info);

        // Check if it's the end of enumeration
        if ((info[1] & 0xFFFF) == 0) {
            break;
        }

        int levelType = (info[2] >> 8) & 0xFF; // Level type
        if (levelType == 1) {
            // SMT level (Simultaneous Multithreading), this gives logical processors per core
            logicalProcessorsPerCore = info[1] & 0xFFFF;
        } else if (levelType == 2) {
            // Core level, this gives the total logical processors per package
            totalLogicalProcessors = info[1] & 0xFFFF;
        }

        subleaf++;
    }

    // Calculate the number of physical cores
    int physicalCores = totalLogicalProcessors / logicalProcessorsPerCore;

    // Output the correct number of cores and logical processors
    std::cout << "Number of Physical Cores: " << physicalCores << std::endl;
    std::cout << "Number of Logical Processors: " << totalLogicalProcessors << std::endl;

    // Get the base and max frequency (leaf 0x16)
    getCPUInfo(0x16, 0, info);
    int baseFreq = info[0]; // Base frequency in MHz
    int maxFreq = info[1];  // Max frequency in MHz
    std::cout << "Base Frequency: " << baseFreq << " MHz" << std::endl;
    std::cout << "Max Frequency: " << maxFreq << " MHz" << std::endl;

    // Check if Turbo Boost is supported (CPUID leaf 6, EAX bit 1)
    getCPUInfo(0x6, 0, info);
    bool turboBoostSupported = info[0] & (1 << 1);
    std::cout << "Turbo Boost 2.0: " << (turboBoostSupported ? "Supported" : "Not Supported") << std::endl;

    // Check for SIMD features in ECX and EDX registers (leaf 1)
    bool sse    = info[3] & (1 << 25);  // EDX bit 25 (SSE)
    bool sse2   = info[3] & (1 << 26);  // EDX bit 26 (SSE2)
    bool sse3   = info[2] & (1 << 0);   // ECX bit 0 (SSE3)
    bool ssse3  = info[2] & (1 << 9);   // ECX bit 9 (SSSE3)
    bool sse41  = info[2] & (1 << 19);  // ECX bit 19 (SSE4.1)
    bool sse42  = info[2] & (1 << 20);  // ECX bit 20 (SSE4.2)
    bool avx    = info[2] & (1 << 28);  // ECX bit 28 (AVX)

    // Output SIMD feature support
    std::cout << "SIMD Support:\n";
    std::cout << "SSE:    "  << (sse ? "Yes" : "No")  << std::endl;
    std::cout << "SSE2:   "  << (sse2 ? "Yes" : "No") << std::endl;
    std::cout << "SSE3:   "  << (sse3 ? "Yes" : "No") << std::endl;
    std::cout << "SSSE3:  "  << (ssse3 ? "Yes" : "No") << std::endl;
    std::cout << "SSE4.1: "  << (sse41 ? "Yes" : "No") << std::endl;
    std::cout << "SSE4.2: "  << (sse42 ? "Yes" : "No") << std::endl;
    std::cout << "AVX:    "  << (avx ? "Yes" : "No")  << std::endl;

    // Check extended features (leaf 7, subleaf 0) for AVX2 and AVX-512
    getCPUInfo(7, 0, info); // Leaf 7 for advanced SIMD features
    bool avx2     = info[1] & (1 << 5);   // EBX bit 5 (AVX2)
    bool avx512f  = info[1] & (1 << 16);  // EBX bit 16 (AVX-512 Foundation)

    // Output AVX2 and AVX-512 support
    std::cout << "AVX2:   "  << (avx2 ? "Yes" : "No") << std::endl;
    std::cout << "AVX-512: "  << (avx512f ? "Yes" : "No") << std::endl;

    return 0;
}
