// File: include/caca/simd/dispatcher.hpp
#pragma once

#include "caca/simd/cpu_features.hpp"
#include <type_traits>
#include <functional>
#include <memory>
#include <stdexcept>

// Include SIMD headers based on platform
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
    #include <arm_neon.h>
#endif

namespace caca::simd {

/**
 * @class SIMDDispatcher
 * @brief Dispatches algorithm execution to the optimal SIMD implementation
 *
 * This class provides runtime detection and dispatch of algorithms to the
 * best available SIMD implementation based on CPU capabilities.
 */
class SIMDDispatcher {
public:
    /**
     * @enum SIMDLevel
     * @brief Enumeration of supported SIMD instruction levels
     */
    enum class SIMDLevel {
        Scalar,     // No SIMD instructions (fallback)
        SSE2,       // SSE2 instructions
        AVX,        // AVX instructions
        AVX2,       // AVX2 instructions
        AVX512F,    // AVX-512 Foundation
        AVX512VNNI, // AVX-512 Vector Neural Network Instructions
        NEON        // ARM NEON
    };

    /**
     * @brief Detect the best available SIMD strategy at runtime
     * @return The highest supported SIMD level
     */
    static SIMDLevel detectBestStrategy() {
        auto features = CPUFeatures::detect();
        
        // Prioritized detection logic from highest to lowest
        if (features->hasFeature(CPUFeatures::Feature::AVX512VNNI)) 
            return SIMDLevel::AVX512VNNI;
        
        if (features->hasFeature(CPUFeatures::Feature::AVX512F)) 
            return SIMDLevel::AVX512F;
        
        if (features->hasFeature(CPUFeatures::Feature::AVX2)) 
            return SIMDLevel::AVX2;
        
        if (features->hasFeature(CPUFeatures::Feature::AVX)) 
            return SIMDLevel::AVX;
        
        if (features->hasFeature(CPUFeatures::Feature::SSE2)) 
            return SIMDLevel::SSE2;
        
        if (features->hasFeature(CPUFeatures::Feature::NEON)) 
            return SIMDLevel::NEON;
        
        return SIMDLevel::Scalar;
    }

    /**
     * @brief Get string representation of SIMD level
     * @param level The SIMD level
     * @return String representation
     */
    static std::string getSIMDLevelName(SIMDLevel level) {
        switch (level) {
            case SIMDLevel::Scalar: return "Scalar";
            case SIMDLevel::SSE2: return "SSE2";
            case SIMDLevel::AVX: return "AVX";
            case SIMDLevel::AVX2: return "AVX2";
            case SIMDLevel::AVX512F: return "AVX-512F";
            case SIMDLevel::AVX512VNNI: return "AVX-512VNNI";
            case SIMDLevel::NEON: return "NEON";
            default: return "Unknown";
        }
    }

    /**
     * @brief Traits for SIMD types based on SIMD level
     * @tparam Level The SIMD level
     */
    template<SIMDLevel Level>
    struct SIMDTraits {
        // Default to scalar (will be specialized for specific SIMD levels)
        using ComputeType = uint64_t;
        static constexpr size_t VectorWidth = 1;
    };

    /**
     * @brief Execute an algorithm with the best available SIMD implementation
     * @tparam Algorithm The algorithm type (must provide a vectorize<T>() method)
     * @param algo Algorithm instance
     * @return Result of algorithm execution
     */
    template<typename Algorithm>
    static auto execute(Algorithm&& algo) {
        switch (detectBestStrategy()) {
        #ifdef __AVX512VNNI__
            case SIMDLevel::AVX512VNNI:
                return algo.template vectorize<__m512i>();
        #endif
        
        #ifdef __AVX512F__
            case SIMDLevel::AVX512F:
                return algo.template vectorize<__m512i>();
        #endif
        
        #ifdef __AVX2__
            case SIMDLevel::AVX2:
                return algo.template vectorize<__m256i>();
        #endif
        
        #ifdef __AVX__
            case SIMDLevel::AVX:
                return algo.template vectorize<__m256i>();
        #endif
        
        #ifdef __SSE2__
            case SIMDLevel::SSE2:
                return algo.template vectorize<__m128i>();
        #endif
        
        #ifdef __ARM_NEON__
            case SIMDLevel::NEON:
                return algo.template vectorize<uint8x16_t>();
        #endif
        
            default:
                return algo.template vectorize<uint64_t>();
        }
    }

    /**
     * @brief Execute an algorithm with a specific SIMD implementation
     * @tparam Level The SIMD level to use
     * @tparam Algorithm The algorithm type (must provide a vectorize<T>() method)
     * @param algo Algorithm instance
     * @return Result of algorithm execution
     */
    template<SIMDLevel Level, typename Algorithm>
    static auto executeWithLevel(Algorithm&& algo) {
        using Traits = SIMDTraits<Level>;
        return algo.template vectorize<typename Traits::ComputeType>();
    }

    /**
     * @brief Benchmarks an algorithm with different SIMD implementations
     * @tparam Algorithm The algorithm type (must provide a vectorize<T>() method)
     * @param algo Algorithm instance
     * @param iterations Number of benchmark iterations
     * @return Map of SIMD level to execution time in microseconds
     */
    template<typename Algorithm>
    static std::unordered_map<SIMDLevel, double> benchmark(
        Algorithm&& algo, 
        size_t iterations = 10
    ) {
        std::unordered_map<SIMDLevel, double> results;
        auto features = CPUFeatures::detect();
        
        // Lambda to run benchmark
        auto runBenchmark = [&](SIMDLevel level, auto&& func) {
            using namespace std::chrono;
            
            // Warmup
            func();
            
            // Benchmark
            auto start = high_resolution_clock::now();
            for (size_t i = 0; i < iterations; ++i) {
                func();
            }
            auto end = high_resolution_clock::now();
            
            // Calculate average time per iteration in microseconds
            auto duration = duration_cast<microseconds>(end - start).count();
            results[level] = static_cast<double>(duration) / iterations;
        };
        
        // Scalar (always available)
        runBenchmark(SIMDLevel::Scalar, [&]() {
            executeWithLevel<SIMDLevel::Scalar>(algo);
        });
        
        // SSE2
        if (features->hasFeature(CPUFeatures::Feature::SSE2)) {
            runBenchmark(SIMDLevel::SSE2, [&]() {
                executeWithLevel<SIMDLevel::SSE2>(algo);
            });
        }
        
        // AVX
        if (features->hasFeature(CPUFeatures::Feature::AVX)) {
            runBenchmark(SIMDLevel::AVX, [&]() {
                executeWithLevel<SIMDLevel::AVX>(algo);
            });
        }
        
        // AVX2
        if (features->hasFeature(CPUFeatures::Feature::AVX2)) {
            runBenchmark(SIMDLevel::AVX2, [&]() {
                executeWithLevel<SIMDLevel::AVX2>(algo);
            });
        }
        
        // AVX-512F
        if (features->hasFeature(CPUFeatures::Feature::AVX512F)) {
            runBenchmark(SIMDLevel::AVX512F, [&]() {
                executeWithLevel<SIMDLevel::AVX512F>(algo);
            });
        }
        
        // AVX-512VNNI
        if (features->hasFeature(CPUFeatures::Feature::AVX512VNNI)) {
            runBenchmark(SIMDLevel::AVX512VNNI, [&]() {
                executeWithLevel<SIMDLevel::AVX512VNNI>(algo);
            });
        }
        
        // NEON
        if (features->hasFeature(CPUFeatures::Feature::NEON)) {
            runBenchmark(SIMDLevel::NEON, [&]() {
                executeWithLevel<SIMDLevel::NEON>(algo);
            });
        }
        
        return results;
    }
};

// SIMD traits specializations

// SSE2 specialization
template<>
struct SIMDDispatcher::SIMDTraits<SIMDDispatcher::SIMDLevel::SSE2> {
#ifdef __SSE2__
    using ComputeType = __m128i;
    static constexpr size_t VectorWidth = 16; // 128 bits = 16 bytes
#else
    using ComputeType = uint64_t;
    static constexpr size_t VectorWidth = 1;
#endif
};

// AVX specialization
template<>
struct SIMDDispatcher::SIMDTraits<SIMDDispatcher::SIMDLevel::AVX> {
#ifdef __AVX__
    using ComputeType = __m256i;
    static constexpr size_t VectorWidth = 32; // 256 bits = 32 bytes
#else
    using ComputeType = uint64_t;
    static constexpr size_t VectorWidth = 1;
#endif
};

// AVX2 specialization
template<>
struct SIMDDispatcher::SIMDTraits<SIMDDispatcher::SIMDLevel::AVX2> {
#ifdef __AVX2__
    using ComputeType = __m256i;
    static constexpr size_t VectorWidth = 32; // 256 bits = 32 bytes
#else
    using ComputeType = uint64_t;
    static constexpr size_t VectorWidth = 1;
#endif
};

// AVX-512F specialization
template<>
struct SIMDDispatcher::SIMDTraits<SIMDDispatcher::SIMDLevel::AVX512F> {
#ifdef __AVX512F__
    using ComputeType = __m512i;
    static constexpr size_t VectorWidth = 64; // 512 bits = 64 bytes
#else
    using ComputeType = uint64_t;
    static constexpr size_t VectorWidth = 1;
#endif
};

// AVX-512VNNI specialization
template<>
struct SIMDDispatcher::SIMDTraits<SIMDDispatcher::SIMDLevel::AVX512VNNI> {
#ifdef __AVX512VNNI__
    using ComputeType = __m512i;
    static constexpr size_t VectorWidth = 64; // 512 bits = 64 bytes
#else
    using ComputeType = uint64_t;
    static constexpr size_t VectorWidth = 1;
#endif
};

// NEON specialization
template<>
struct SIMDDispatcher::SIMDTraits<SIMDDispatcher::SIMDLevel::NEON> {
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    using ComputeType = uint8x16_t;
    static constexpr size_t VectorWidth = 16; // 128 bits = 16 bytes
#else
    using ComputeType = uint64_t;
    static constexpr size_t VectorWidth = 1;
#endif
};

} // namespace caca::simd

// File: src/simd/dispatcher.cpp
#include "caca/simd/dispatcher.hpp"

namespace caca::simd {
    // Implementation of any non-template methods would go here
    // Currently all methods are templated and defined in the header
}