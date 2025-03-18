// File: include/caca/ca/cellular_automata.hpp
#pragma once

#include "caca/core/bitsequence.hpp"
#include "caca/simd/dispatcher.hpp"
#include "caca/core/error_handling.hpp"
#include <vector>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <future>

namespace caca::ca {

/**
 * @enum CARule
 * @brief Predefined cellular automata rules
 */
enum class CARule : uint8_t {
    Rule30 = 30,   // Chaotic rule
    Rule82 = 82,   // Random-like rule
    Rule110 = 110, // Universal computation capable rule
    Rule150 = 150, // Linear rule
    Custom = 0     // User-defined rule
};

/**
 * @enum CANeighborhood
 * @brief Type of cellular automata neighborhood
 */
enum class CANeighborhood {
    Moore,         // 8 neighbors (including diagonals)
    VonNeumann,    // 4 neighbors (cardinal directions only)
    OneDimensional // 2 neighbors (left and right only)
};

/**
 * @class CellularAutomataProcessor
 * @brief Processes data using cellular automata rules
 * 
 * This class implements various cellular automata rules to process
 * binary data, similar to the technique used in image denoising but
 * applied to potentially encrypted data to detect patterns.
 */
class CellularAutomataProcessor {
private:
    // The bit sequence to process
    BitSequence m_data;
    
    // Buffer for the next iteration
    BitSequence m_buffer;
    
    // Current rule
    CARule m_rule;
    
    // Custom rule function (if using CARule::Custom)
    std::function<bool(const BitSequence&, size_t)> m_custom_rule_func;
    
    // Neighborhood type
    CANeighborhood m_neighborhood;
    
    // Width and height for 2D interpretation
    size_t m_width;
    size_t m_height;
    
    // Number of threads to use for processing
    size_t m_num_threads;
    
    // Apply CA rule to a specific position using 1D neighborhood
    bool apply1DRule(size_t pos) const;
    
    // Apply CA rule to a specific position using Von Neumann neighborhood
    bool applyVonNeumannRule(size_t pos) const;
    
    // Apply CA rule to a specific position using Moore neighborhood
    bool applyMooreRule(size_t pos) const;
    
    // Update the buffer with a single iteration
    void updateBuffer();
    
    // Parallel worker method
    void workerThread(size_t start_idx, size_t end_idx);
    
    // Get index from 2D coordinates
    size_t getIndex(size_t x, size_t y) const {
        return y * m_width + x;
    }
    
    // Get 2D coordinates from index
    std::pair<size_t, size_t> getCoordinates(size_t index) const {
        return { index % m_width, index / m_width };
    }
    
    // Helper method to apply standard 1D CA rules
    bool applyStandardRule(uint8_t pattern) const;

public:
    /**
     * @brief Constructor with bit sequence
     * @param data Input bit sequence
     * @param rule CA rule to apply
     * @param neighborhood Neighborhood type
     * @param width Width for 2D interpretation (0 for automatic square)
     */
    CellularAutomataProcessor(
        const BitSequence& data,
        CARule rule = CARule::Rule30,
        CANeighborhood neighborhood = CANeighborhood::Moore,
        size_t width = 0
    );
    
    /**
     * @brief Constructor with byte sequence
     * @param data Input byte sequence
     * @param rule CA rule to apply
     * @param neighborhood Neighborhood type
     * @param width Width for 2D interpretation (0 for automatic square)
     */
    CellularAutomataProcessor(
        const ByteSequence& data,
        CARule rule = CARule::Rule30,
        CANeighborhood neighborhood = CANeighborhood::Moore,
        size_t width = 0
    );
    
    /**
     * @brief Set the rule to use
     * @param rule CA rule
     */
    void setRule(CARule rule) {
        m_rule = rule;
    }
    
    /**
     * @brief Set a custom rule function
     * @param rule_func Function that determines the next state
     */
    void setCustomRule(std::function<bool(const BitSequence&, size_t)> rule_func) {
        m_rule = CARule::Custom;
        m_custom_rule_func = rule_func;
    }
    
    /**
     * @brief Set the neighborhood type
     * @param neighborhood Neighborhood type
     */
    void setNeighborhood(CANeighborhood neighborhood) {
        m_neighborhood = neighborhood;
    }
    
    /**
     * @brief Set the number of threads to use
     * @param num_threads Number of threads (0 for automatic)
     */
    void setNumThreads(size_t num_threads) {
        m_num_threads = (num_threads > 0) ? 
            num_threads : std::thread::hardware_concurrency();
    }
    
    /**
     * @brief Process the data with multiple CA iterations
     * @param iterations Number of iterations to apply
     * @return Processed bit sequence
     */
    BitSequence process(size_t iterations = 1);
    
    /**
     * @brief Process byte data with multiple CA iterations
     * @param iterations Number of iterations to apply
     * @return Processed byte sequence
     */
    ByteSequence processBytes(size_t iterations = 1);
    
    /**
     * @brief Get the rule name as a string
     * @return Rule name
     */
    std::string getRuleName() const;
    
    /**
     * @brief Get the original data
     * @return Reference to the original bit sequence
     */
    const BitSequence& getData() const {
        return m_data;
    }
    
    /**
     * @brief Get the current width
     * @return Width for 2D interpretation
     */
    size_t getWidth() const {
        return m_width;
    }
    
    /**
     * @brief Get the current height
     * @return Height for 2D interpretation
     */
    size_t getHeight() const {
        return m_height;
    }
};

// SIMD implementations for CA processing
namespace simd {

/**
 * @class CellularAutomataKernel
 * @brief SIMD-optimized algorithm for CA processing
 * 
 * This class implements SIMD-accelerated cellular automata
 * rule applications using various SIMD instruction sets.
 */
class CellularAutomataKernel {
private:
    const BitSequence* m_data;
    BitSequence* m_buffer;
    CARule m_rule;
    CANeighborhood m_neighborhood;
    size_t m_width;
    size_t m_height;
    
public:
    /**
     * @brief Constructor
     * @param data Input data
     * @param buffer Output buffer
     * @param rule CA rule to apply
     * @param neighborhood Neighborhood type
     * @param width Width for 2D interpretation
     * @param height Height for 2D interpretation
     */
    CellularAutomataKernel(
        const BitSequence* data,
        BitSequence* buffer,
        CARule rule,
        CANeighborhood neighborhood,
        size_t width,
        size_t height
    ) : m_data(data), m_buffer(buffer), m_rule(rule),
        m_neighborhood(neighborhood), m_width(width), m_height(height) {}

    /**
     * @brief Process data using SIMD instructions
     * @tparam VectorType SIMD vector type
     * @param start_idx Start index
     * @param end_idx End index
     */
    template<typename VectorType>
    void vectorize(size_t start_idx, size_t end_idx);
};

} // namespace simd

} // namespace caca::ca

// File: src/ca/cellular_automata.cpp
#include "caca/ca/cellular_automata.hpp"
#include "caca/monitoring/progress_monitor.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <future>

namespace caca::ca {

CellularAutomataProcessor::CellularAutomataProcessor(
    const BitSequence& data,
    CARule rule,
    CANeighborhood neighborhood,
    size_t width
) : m_data(data), m_buffer(data.size()), m_rule(rule),
    m_neighborhood(neighborhood), m_num_threads(std::thread::hardware_concurrency()) {
    
    // Determine dimensions for 2D interpretation
    if (width == 0) {
        // Auto-calculate square dimensions
        m_width = static_cast<size_t>(std::sqrt(data.size()));
        m_height = (data.size() + m_width - 1) / m_width;
    } else {
        m_width = width;
        m_height = (data.size() + width - 1) / width;
    }
    
    // Ensure buffer is the right size
    m_buffer.resize(m_data.size());
}

CellularAutomataProcessor::CellularAutomataProcessor(
    const ByteSequence& data,
    CARule rule,
    CANeighborhood neighborhood,
    size_t width
) : m_data(data), m_buffer(m_data.size()), m_rule(rule),
    m_neighborhood(neighborhood), m_num_threads(std::thread::hardware_concurrency()) {
    
    // Determine dimensions for 2D interpretation
    if (width == 0) {
        // Auto-calculate square dimensions
        m_width = static_cast<size_t>(std::sqrt(m_data.size()));
        m_height = (m_data.size() + m_width - 1) / m_width;
    } else {
        m_width = width;
        m_height = (m_data.size() + width - 1) / width;
    }
    
    // Ensure buffer is the right size
    m_buffer.resize(m_data.size());
}

bool CellularAutomataProcessor::applyStandardRule(uint8_t pattern) const {
    uint8_t rule_value = static_cast<uint8_t>(m_rule);
    return (rule_value & (1 << pattern)) != 0;
}

bool CellularAutomataProcessor::apply1DRule(size_t pos) const {
    if (m_rule == CARule::Custom) {
        return m_custom_rule_func(m_data, pos);
    }
    
    // For 1D cellular automata, we use a 3-cell neighborhood
    uint8_t pattern = 0;
    
    // Left neighbor (or wrap around)
    if (pos > 0) {
        pattern |= m_data[pos - 1] ? 4 : 0;
    } else {
        pattern |= m_data[m_data.size() - 1] ? 4 : 0;
    }
    
    // Center cell
    pattern |= m_data[pos] ? 2 : 0;
    
    // Right neighbor (or wrap around)
    if (pos < m_data.size() - 1) {
        pattern |= m_data[pos + 1] ? 1 : 0;
    } else {
        pattern |= m_data[0] ? 1 : 0;
    }
    
    return applyStandardRule(pattern);
}

bool CellularAutomataProcessor::applyVonNeumannRule(size_t pos) const {
    if (m_rule == CARule::Custom) {
        return m_custom_rule_func(m_data, pos);
    }
    
    auto [x, y] = getCoordinates(pos);
    
    // Count living neighbors (Von Neumann neighborhood: 4 neighbors)
    size_t living_neighbors = 0;
    
    // North
    if (y > 0 && m_data[getIndex(x, y - 1)]) {
        living_neighbors++;
    }
    
    // East
    if (x < m_width - 1 && m_data[getIndex(x + 1, y)]) {
        living_neighbors++;
    }
    
    // South
    if (y < m_height - 1 && m_data[getIndex(x, y + 1)]) {
        living_neighbors++;
    }
    
    // West
    if (x > 0 && m_data[getIndex(x - 1, y)]) {
        living_neighbors++;
    }
    
    // For Von Neumann neighborhood, use a simple majority rule
    bool alive = m_data[pos];
    
    // Implement different rules
    switch (m_rule) {
        case CARule::Rule30:
            return (alive && living_neighbors < 2) || (!alive && living_neighbors >= 2);
        
        case CARule::Rule82:
            return (alive && living_neighbors < 3) || (!alive && living_neighbors == 2);
        
        case CARule::Rule110:
            return (alive && living_neighbors != 4) || (!alive && living_neighbors >= 1);
        
        case CARule::Rule150:
            // XOR rule - flip if odd number of living neighbors
            return (living_neighbors % 2) != 0;
        
        default:
            return alive; // Default just keep current state
    }
}

bool CellularAutomataProcessor::applyMooreRule(size_t pos) const {
    if (m_rule == CARule::Custom) {
        return m_custom_rule_func(m_data, pos);
    }
    
    auto [x, y] = getCoordinates(pos);
    
    // Count living neighbors (Moore neighborhood: 8 neighbors)
    size_t living_neighbors = 0;
    
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            // Skip the center cell
            if (dx == 0 && dy == 0) continue;
            
            // Calculate neighbor coordinates with boundary checking
            int nx = static_cast<int>(x) + dx;
            int ny = static_cast<int>(y) + dy;
            
            if (nx >= 0 && nx < static_cast<int>(m_width) && 
                ny >= 0 && ny < static_cast<int>(m_height)) {
                if (m_data[getIndex(nx, ny)]) {
                    living_neighbors++;
                }
            }
        }
    }
    
    bool alive = m_data[pos];
    
    // Implement different rules
    switch (m_rule) {
        case CARule::Rule30:
            // For Moore neighborhood, adapt Rule 30 (chaos):
            // A cell is alive if exactly 3 neighbors are alive or if it was alive and has 2 living neighbors
            return living_neighbors == 3 || (alive && living_neighbors == 2);
        
        case CARule::Rule82:
            // For Moore neighborhood, adapt Rule 82 (randomizing):
            // A cell is alive if it was dead and has exactly 3 living neighbors, or it was alive and has 2-3 living neighbors
            return (!alive && living_neighbors == 3) || (alive && (living_neighbors == 2 || living_neighbors == 3));
        
        case CARule::Rule110:
            // For Moore neighborhood, adapt Rule 110 (computation):
            // Complex rule that can support universal computation
            return (alive && living_neighbors < 4) || (!alive && (living_neighbors == 3 || living_neighbors == 6));
        
        case CARule::Rule150:
            // For Moore neighborhood, adapt Rule 150 (linear, reversible):
            // XOR rule based on Moore neighborhood
            return (living_neighbors % 2) != 0;
        
        default:
            return alive; // Default just keep current state
    }
}

void CellularAutomataProcessor::updateBuffer() {
    // Use SIMD optimization if possible
    simd::CellularAutomataKernel kernel(
        &m_data, &m_buffer, m_rule, m_neighborhood, m_width, m_height
    );
    
    // Create workers based on number of threads
    std::vector<std::future<void>> futures;
    futures.reserve(m_num_threads);
    
    size_t chunk_size = m_data.size() / m_num_threads;
    for (size_t i = 0; i < m_num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i == m_num_threads - 1) ? m_data.size() : (i + 1) * chunk_size;
        
        futures.push_back(std::async(
            std::launch::async,
            [&kernel, start, end]() {
                // Dispatch to best SIMD implementation
                simd::SIMDDispatcher::execute(
                    [&kernel, start, end](auto&&... args) {
                        return kernel.vectorize<std::decay_t<decltype(args)>...>(start, end);
                    }
                );
                // This would call the vectorize method with the appropriate
                // SIMD vector type based on CPU capabilities
            }
        ));
    }
    
    // Wait for all workers to finish
    for (auto& future : futures) {
        future.wait();
    }
}

void CellularAutomataProcessor::workerThread(size_t start_idx, size_t end_idx) {
    for (size_t i = start_idx; i < end_idx; ++i) {
        switch (m_neighborhood) {
            case CANeighborhood::OneDimensional:
                m_buffer[i] = apply1DRule(i);
                break;
            
            case CANeighborhood::VonNeumann:
                m_buffer[i] = applyVonNeumannRule(i);
                break;
            
            case CANeighborhood::Moore:
                m_buffer[i] = applyMooreRule(i);
                break;
        }
    }
}

BitSequence CellularAutomataProcessor::process(size_t iterations) {
    if (iterations == 0) {
        return m_data; // No processing needed
    }
    
    // Create progress monitor for better user feedback
    monitoring::ProgressMonitor monitor;
    
    for (size_t iter = 0; iter < iterations; ++iter) {
        // Update progress information
        monitor.update_progress(
            "Cellular Automata Processing", 
            iter, 
            iterations
        );
        
        // Process one iteration
        updateBuffer();
        
        // Swap buffers for next iteration
        std::swap(m_data, m_buffer);
    }
    
    // Final progress update
    monitor.update_progress(
        "Cellular Automata Processing", 
        iterations, 
        iterations
    );
    
    return m_data;
}

ByteSequence CellularAutomataProcessor::processBytes(size_t iterations) {
    BitSequence result = process(iterations);
    return result.toByteVector();
}

std::string CellularAutomataProcessor::getRuleName() const {
    switch (m_rule) {
        case CARule::Rule30:
            return "Rule 30 (Chaotic)";
        
        case CARule::Rule82:
            return "Rule 82 (Random-like)";
        
        case CARule::Rule110:
            return "Rule 110 (Universal)";
        
        case CARule::Rule150:
            return "Rule 150 (Linear)";
        
        case CARule::Custom:
            return "Custom Rule";
        
        default: {
            std::stringstream ss;
            ss << "Rule " << static_cast<int>(m_rule);
            return ss.str();
        }
    }
}

namespace simd {

// Scalar (non-SIMD) implementation
template<>
void CellularAutomataKernel::vectorize<uint64_t>(size_t start_idx, size_t end_idx) {
    for (size_t i = start_idx; i < end_idx; ++i) {
        bool next_state = false;
        
        // Apply rule based on neighborhood type
        switch (m_neighborhood) {
            case CANeighborhood::OneDimensional: {
                // For 1D cellular automata, we use a 3-cell neighborhood
                uint8_t pattern = 0;
                
                // Left neighbor (or wrap around)
                if (i > 0) {
                    pattern |= (*m_data)[i - 1] ? 4 : 0;
                } else {
                    pattern |= (*m_data)[m_data->size() - 1] ? 4 : 0;
                }
                
                // Center cell
                pattern |= (*m_data)[i] ? 2 : 0;
                
                // Right neighbor (or wrap around)
                if (i < m_data->size() - 1) {
                    pattern |= (*m_data)[i + 1] ? 1 : 0;
                } else {
                    pattern |= (*m_data)[0] ? 1 : 0;
                }
                
                uint8_t rule_value = static_cast<uint8_t>(m_rule);
                next_state = (rule_value & (1 << pattern)) != 0;
                break;
            }
            
            case CANeighborhood::VonNeumann: {
                size_t x = i % m_width;
                size_t y = i / m_width;
                
                // Count living neighbors (Von Neumann neighborhood: 4 neighbors)
                size_t living_neighbors = 0;
                
                // North
                if (y > 0 && (*m_data)[y > 0 ? (y - 1) * m_width + x : i]) {
                    living_neighbors++;
                }
                
                // East
                if (x < m_width - 1 && (*m_data)[y * m_width + (x < m_width - 1 ? x + 1 : x)]) {
                    living_neighbors++;
                }
                
                // South
                if (y < m_height - 1 && (*m_data)[y < m_height - 1 ? (y + 1) * m_width + x : i]) {
                    living_neighbors++;
                }
                
                // West
                if (x > 0 && (*m_data)[y * m_width + (x > 0 ? x - 1 : x)]) {
                    living_neighbors++;
                }
                
                bool alive = (*m_data)[i];
                
                // Apply rule based on living neighbors
                switch (m_rule) {
                    case CARule::Rule30:
                        next_state = (alive && living_neighbors < 2) || 
                                     (!alive && living_neighbors >= 2);
                        break;
                    
                    case CARule::Rule82:
                        next_state = (alive && living_neighbors < 3) || 
                                     (!alive && living_neighbors == 2);
                        break;
                    
                    case CARule::Rule110:
                        next_state = (alive && living_neighbors != 4) || 
                                     (!alive && living_neighbors >= 1);
                        break;
                    
                    case CARule::Rule150:
                        next_state = (living_neighbors % 2) != 0;
                        break;
                    
                    default:
                        next_state = alive;
                        break;
                }
                break;
            }
            
            case CANeighborhood::Moore: {
                size_t x = i % m_width;
                size_t y = i / m_width;
                
                // Count living neighbors (Moore neighborhood: 8 neighbors)
                size_t living_neighbors = 0;
                
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        // Skip the center cell
                        if (dx == 0 && dy == 0) continue;
                        
                        // Calculate neighbor coordinates with boundary checking
                        int nx = static_cast<int>(x) + dx;
                        int ny = static_cast<int>(y) + dy;
                        
                        if (nx >= 0 && nx < static_cast<int>(m_width) && 
                            ny >= 0 && ny < static_cast<int>(m_height)) {
                            if ((*m_data)[ny * m_width + nx]) {
                                living_neighbors++;
                            }
                        }
                    }
                }
                
                bool alive = (*m_data)[i];
                
                // Apply rule based on living neighbors
                switch (m_rule) {
                    case CARule::Rule30:
                        next_state = living_neighbors == 3 || 
                                    (alive && living_neighbors == 2);
                        break;
                    
                    case CARule::Rule82:
                        next_state = (!alive && living_neighbors == 3) || 
                                     (alive && (living_neighbors == 2 || living_neighbors == 3));
                        break;
                    
                    case CARule::Rule110:
                        next_state = (alive && living_neighbors < 4) || 
                                     (!alive && (living_neighbors == 3 || living_neighbors == 6));
                        break;
                    
                    case CARule::Rule150:
                        next_state = (living_neighbors % 2) != 0;
                        break;
                    
                    default:
                        next_state = alive;
                        break;
                }
                break;
            }
        }
        
        // Update buffer
        (*m_buffer)[i] = next_state;
    }
}

// SSE2 Implementation
#ifdef __SSE2__
template<>
void CellularAutomataKernel::vectorize<__m128i>(size_t start_idx, size_t end_idx) {
    // Implementation would go here
    // For brevity, just fall back to scalar version for now
    vectorize<uint64_t>(start_idx, end_idx);
}
#endif

// AVX2 Implementation
#ifdef __AVX2__
template<>
void CellularAutomataKernel::vectorize<__m256i>(size_t start_idx, size_t end_idx) {
    // Implementation would go here
    // For brevity, just fall back to scalar version for now
    vectorize<uint64_t>(start_idx, end_idx);
}
#endif

// AVX-512 Implementation
#ifdef __AVX512F__
template<>
void CellularAutomataKernel::vectorize<__m512i>(size_t start_idx, size_t end_idx) {
    // Implementation would go here
    // For brevity, just fall back to scalar version for now
    vectorize<uint64_t>(start_idx, end_idx);
}
#endif

// ARM NEON Implementation
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
template<>
void CellularAutomataKernel::vectorize<uint8x16_t>(size_t start_idx, size_t end_idx) {
    // Implementation would go here
    // For brevity, just fall back to scalar version for now
    vectorize<uint64_t>(start_idx, end_idx);
}
#endif

} // namespace simd

} // namespace caca::ca