// File: include/caca/simd/cpu_features.hpp
#pragma once

#include <memory>
#include <bitset>
#include <array>
#include <string>
#include <vector>
#include <iostream>

namespace caca::simd {

/**
 * @class CPUFeatures
 * @brief Detects and provides information about available CPU SIMD capabilities
 *
 * This class uses CPUID to detect available SIMD instruction sets on x86/x64 CPUs,
 * including SSE, AVX, AVX2, AVX-512 and its variants. It also detects ARM NEON
 * on compatible platforms.
 */
class CPUFeatures {
public:
    /**
     * @enum Feature
     * @brief Enumeration of supported CPU features
     */
    enum class Feature {
        SSE,
        SSE2,
        SSE3,
        SSSE3,
        SSE41,
        SSE42,
        AVX,
        AVX2,
        AVX512F,      // Foundation
        AVX512DQ,     // Doubleword and Quadword
        AVX512IFMA,   // Integer Fused Multiply-Add
        AVX512PF,     // Prefetch
        AVX512ER,     // Exponential and Reciprocal
        AVX512CD,     // Conflict Detection
        AVX512BW,     // Byte and Word
        AVX512VL,     // Vector Length Extensions
        AVX512VBMI,   // Vector Byte Manipulation Instructions
        AVX512VBMI2,  // Vector Byte Manipulation Instructions 2
        AVX512VNNI,   // Vector Neural Network Instructions
        AVX512BITALG, // Bit Algorithms
        AVX512VPOPCNTDQ, // Vector Population Count
        NEON,         // ARM NEON
        COUNT         // Keep last - used for feature count
    };

private:
    // BitSet to store which features are available
    std::bitset<static_cast<size_t>(Feature::COUNT)> m_features;
    static std::shared_ptr<CPUFeatures> s_instance;

    // Maps Feature enum to string representation
    static const std::array<std::string, static_cast<size_t>(Feature::COUNT)> s_feature_names;

    // Private constructor - use detect() to create instance
    CPUFeatures();

    // Initialize detected features
    void detectFeatures();

    // CPUID helper functions
    static void cpuid(int info[4], int func_id, int sub_func_id = 0);
    static bool has_cpuid_support();

public:
    /**
     * @brief Get the CPU features singleton instance
     * @return Shared pointer to the CPUFeatures instance
     */
    static std::shared_ptr<CPUFeatures> detect();

    /**
     * @brief Check if a specific feature is available
     * @param feature The feature to check
     * @return True if the feature is supported, false otherwise
     */
    bool hasFeature(Feature feature) const {
        return m_features[static_cast<size_t>(feature)];
    }

    /**
     * @brief Get a list of all supported features
     * @return Vector of supported feature names
     */
    std::vector<std::string> getSupportedFeatures() const {
        std::vector<std::string> result;
        for (size_t i = 0; i < static_cast<size_t>(Feature::COUNT); i++) {
            if (m_features[i]) {
                result.push_back(s_feature_names[i]);
            }
        }
        return result;
    }

    /**
     * @brief Get the highest supported SIMD level
     * @return The most advanced SIMD feature available
     */
    Feature getHighestSIMDSupport() const;

    /**
     * @brief Print detailed CPU feature information to stdout
     */
    void printFeatureInfo() const;
};

// Static member initialization
const std::array<std::string, static_cast<size_t>(CPUFeatures::Feature::COUNT)> 
CPUFeatures::s_feature_names = {
    "SSE", "SSE2", "SSE3", "SSSE3", "SSE4.1", "SSE4.2", "AVX", "AVX2", 
    "AVX-512F", "AVX-512DQ", "AVX-512IFMA", "AVX-512PF", "AVX-512ER", 
    "AVX-512CD", "AVX-512BW", "AVX-512VL", "AVX-512VBMI", "AVX-512VBMI2", 
    "AVX-512VNNI", "AVX-512BITALG", "AVX-512VPOPCNTDQ", "NEON"
};

std::shared_ptr<CPUFeatures> CPUFeatures::s_instance = nullptr;

} // namespace caca::simd

// File: src/simd/cpu_features.cpp
#include "caca/simd/cpu_features.hpp"

#if defined(_MSC_VER)
    #include <intrin.h>
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    #include <cpuid.h>
    #include <x86intrin.h>
#endif

#include <cstring>

namespace caca::simd {

CPUFeatures::CPUFeatures() {
    detectFeatures();
}

std::shared_ptr<CPUFeatures> CPUFeatures::detect() {
    if (!s_instance) {
        s_instance = std::shared_ptr<CPUFeatures>(new CPUFeatures());
    }
    return s_instance;
}

void CPUFeatures::cpuid(int info[4], int func_id, int sub_func_id) {
#if defined(_MSC_VER)
    __cpuidex(info, func_id, sub_func_id);
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    __cpuid_count(func_id, sub_func_id, info[0], info[1], info[2], info[3]);
#else
    // Zero out the info array for unsupported platforms
    memset(info, 0, sizeof(int) * 4);
#endif
}

bool CPUFeatures::has_cpuid_support() {
#if defined(_MSC_VER) && defined(_M_IX86)
    // For 32-bit x86 on MSVC
    int cpuInfo[4] = {0};
    __cpuid(cpuInfo, 0);
    return cpuInfo[0] != 0;
#elif defined(__GNUC__) && defined(__i386__)
    // For 32-bit x86 on GCC/Clang
    int has_cpuid = 0;
    asm volatile(
        "pushfl\n\t"
        "pop %%eax\n\t"
        "mov %%eax, %%ecx\n\t"
        "xor $0x200000, %%eax\n\t"
        "push %%eax\n\t"
        "popfl\n\t"
        "pushfl\n\t"
        "pop %%eax\n\t"
        "xor %%ecx, %%eax\n\t"
        "mov $0, %0\n\t"
        "test $0x200000, %%eax\n\t"
        "jz 1f\n\t"
        "mov $1, %0\n\t"
        "1:\n\t"
        : "=r" (has_cpuid)
        :
        : "eax", "ecx", "cc"
    );
    return has_cpuid;
#else
    // For x86_64, ARM64, and other platforms
    // CPUID is always supported on x86_64
    // ARM64 doesn't use CPUID
    return true;
#endif
}

void CPUFeatures::detectFeatures() {
    m_features.reset(); // Start with no features

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    // x86/x64 architecture
    if (!has_cpuid_support()) {
        return;
    }

    int info[4];
    
    // Get vendor string
    cpuid(info, 0);
    int nIds = info[0];

    // Check SSE features
    if (nIds >= 1) {
        cpuid(info, 1);
        m_features[static_cast<size_t>(Feature::SSE)]    = (info[3] & (1 << 25)) != 0;
        m_features[static_cast<size_t>(Feature::SSE2)]   = (info[3] & (1 << 26)) != 0;
        m_features[static_cast<size_t>(Feature::SSE3)]   = (info[2] & (1 << 0)) != 0;
        m_features[static_cast<size_t>(Feature::SSSE3)]  = (info[2] & (1 << 9)) != 0;
        m_features[static_cast<size_t>(Feature::SSE41)]  = (info[2] & (1 << 19)) != 0;
        m_features[static_cast<size_t>(Feature::SSE42)]  = (info[2] & (1 << 20)) != 0;
        m_features[static_cast<size_t>(Feature::AVX)]    = (info[2] & (1 << 28)) != 0;
    }

    // Check AVX2 and AVX512 features
    if (nIds >= 7) {
        cpuid(info, 7, 0);
        m_features[static_cast<size_t>(Feature::AVX2)]         = (info[1] & (1 << 5)) != 0;
        m_features[static_cast<size_t>(Feature::AVX512F)]      = (info[1] & (1 << 16)) != 0;
        m_features[static_cast<size_t>(Feature::AVX512DQ)]     = (info[1] & (1 << 17)) != 0;
        m_features[static_cast<size_t>(Feature::AVX512IFMA)]   = (info[1] & (1 << 21)) != 0;
        m_features[static_cast<size_t>(Feature::AVX512PF)]     = (info[1] & (1 << 26)) != 0;
        m_features[static_cast<size_t>(Feature::AVX512ER)]     = (info[1] & (1 << 27)) != 0;
        m_features[static_cast<size_t>(Feature::AVX512CD)]     = (info[1] & (1 << 28)) != 0;
        m_features[static_cast<size_t>(Feature::AVX512BW)]     = (info[1] & (1 << 30)) != 0;
        m_features[static_cast<size_t>(Feature::AVX512VL)]     = (info[1] & (1 << 31)) != 0;
        m_features[static_cast<size_t>(Feature::AVX512VBMI)]   = (info[2] & (1 << 1)) != 0;
        m_features[static_cast<size_t>(Feature::AVX512VBMI2)]  = (info[2] & (1 << 6)) != 0;
        m_features[static_cast<size_t>(Feature::AVX512VNNI)]   = (info[2] & (1 << 11)) != 0;
        m_features[static_cast<size_t>(Feature::AVX512BITALG)] = (info[2] & (1 << 12)) != 0;
        m_features[static_cast<size_t>(Feature::AVX512VPOPCNTDQ)] = (info[2] & (1 << 14)) != 0;
    }
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
    // ARM architecture with NEON
    m_features[static_cast<size_t>(Feature::NEON)] = true;
#endif
}

CPUFeatures::Feature CPUFeatures::getHighestSIMDSupport() const {
    // Check from highest to lowest
    if (hasFeature(Feature::AVX512VNNI))       return Feature::AVX512VNNI;
    if (hasFeature(Feature::AVX512VPOPCNTDQ))  return Feature::AVX512VPOPCNTDQ;
    if (hasFeature(Feature::AVX512BITALG))     return Feature::AVX512BITALG;
    if (hasFeature(Feature::AVX512VBMI2))      return Feature::AVX512VBMI2;
    if (hasFeature(Feature::AVX512VBMI))       return Feature::AVX512VBMI;
    if (hasFeature(Feature::AVX512VL))         return Feature::AVX512VL;
    if (hasFeature(Feature::AVX512BW))         return Feature::AVX512BW;
    if (hasFeature(Feature::AVX512CD))         return Feature::AVX512CD;
    if (hasFeature(Feature::AVX512ER))         return Feature::AVX512ER;
    if (hasFeature(Feature::AVX512PF))         return Feature::AVX512PF;
    if (hasFeature(Feature::AVX512IFMA))       return Feature::AVX512IFMA;
    if (hasFeature(Feature::AVX512DQ))         return Feature::AVX512DQ;
    if (hasFeature(Feature::AVX512F))          return Feature::AVX512F;
    if (hasFeature(Feature::AVX2))             return Feature::AVX2;
    if (hasFeature(Feature::AVX))              return Feature::AVX;
    if (hasFeature(Feature::SSE42))            return Feature::SSE42;
    if (hasFeature(Feature::SSE41))            return Feature::SSE41;
    if (hasFeature(Feature::SSSE3))            return Feature::SSSE3;
    if (hasFeature(Feature::SSE3))             return Feature::SSE3;
    if (hasFeature(Feature::SSE2))             return Feature::SSE2;
    if (hasFeature(Feature::SSE))              return Feature::SSE;
    if (hasFeature(Feature::NEON))             return Feature::NEON;
    
    // No SIMD support detected
    return static_cast<Feature>(static_cast<size_t>(Feature::COUNT) - 1);
}

void CPUFeatures::printFeatureInfo() const {
    std::cout << "CPU SIMD Feature Support:" << std::endl;
    std::cout << "-------------------------" << std::endl;
    
    for (size_t i = 0; i < static_cast<size_t>(Feature::COUNT); i++) {
        std::cout << s_feature_names[i] << ": " 
                  << (m_features[i] ? "Supported" : "Not supported") 
                  << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Highest SIMD support: ";
    Feature highest = getHighestSIMDSupport();
    
    if (highest != static_cast<Feature>(static_cast<size_t>(Feature::COUNT) - 1)) {
        std::cout << s_feature_names[static_cast<size_t>(highest)] << std::endl;
    } else {
        std::cout << "None" << std::endl;
    }
}

} // namespace caca::simd