// File: include/caca/tests/nist_tests.hpp
#pragma once

#include "caca/tests/statistical_tests.hpp"
#include "caca/core/bitsequence.hpp"
#include <string>
#include <vector>
#include <cmath>
#include <complex>
#include <algorithm>
#include <numeric>

namespace caca::tests::nist {

/**
 * @class FrequencyTest
 * @brief NIST Frequency (Monobit) Test
 * 
 * Tests whether the number of 0s and 1s in the sequence are approximately
 * equal, as would be expected from a truly random sequence.
 */
class FrequencyTest : public StatisticalTest {
public:
    /**
     * @brief Execute the Frequency Test
     * @param data Input bit sequence
     * @return Test result
     */
    TestResult execute(const BitSequence& data) override;
    
    /**
     * @brief Get test name
     * @return Test name
     */
    std::string getName() const override {
        return "Frequency (Monobit) Test";
    }
};

/**
 * @class BlockFrequencyTest
 * @brief NIST Block Frequency Test
 * 
 * Tests whether the frequency of 1s in blocks of a specified length
 * is approximately M/2, as would be expected under the assumption of randomness.
 */
class BlockFrequencyTest : public StatisticalTest {
private:
    size_t m_block_size;
    
public:
    /**
     * @brief Constructor
     * @param block_size Size of each block (default 128)
     */
    explicit BlockFrequencyTest(size_t block_size = 128) : m_block_size(block_size) {}
    
    /**
     * @brief Execute the Block Frequency Test
     * @param data Input bit sequence
     * @return Test result
     */
    TestResult execute(const BitSequence& data) override;
    
    /**
     * @brief Get test name
     * @return Test name
     */
    std::string getName() const override {
        return "Block Frequency Test";
    }
    
    /**
     * @brief Set block size
     * @param block_size New block size
     */
    void setBlockSize(size_t block_size) {
        m_block_size = block_size;
    }
    
    /**
     * @brief Get current block size
     * @return Block size
     */
    size_t getBlockSize() const {
        return m_block_size;
    }
};

/**
 * @class RunsTest
 * @brief NIST Runs Test
 * 
 * Tests whether the number of runs (uninterrupted sequences of identical bits)
 * is as expected for a random sequence.
 */
class RunsTest : public StatisticalTest {
public:
    /**
     * @brief Execute the Runs Test
     * @param data Input bit sequence
     * @return Test result
     */
    TestResult execute(const BitSequence& data) override;
    
    /**
     * @brief Get test name
     * @return Test name
     */
    std::string getName() const override {
        return "Runs Test";
    }
};

/**
 * @class LongestRunTest
 * @brief NIST Longest Run of Ones Test
 * 
 * Tests whether the length of the longest run of ones is consistent
 * with the expected length for a random sequence.
 */
class LongestRunTest : public StatisticalTest {
private:
    // Tables for different sequence lengths
    static const std::vector<size_t> M_8;
    static const std::vector<size_t> M_128;
    static const std::vector<size_t> M_10000;
    static const std::vector<double> V_8;
    static const std::vector<double> V_128;
    static const std::vector<double> V_10000;

public:
    /**
     * @brief Execute the Longest Run Test
     * @param data Input bit sequence
     * @return Test result
     */
    TestResult execute(const BitSequence& data) override;
    
    /**
     * @brief Get test name
     * @return Test name
     */
    std::string getName() const override {
        return "Longest Run of Ones Test";
    }
};

/**
 * @class DFTTest
 * @brief NIST Discrete Fourier Transform (Spectral) Test
 * 
 * Tests whether the spectral frequency of the bit sequence shows
 * characteristics different from that of a truly random sequence.
 */
class DFTTest : public StatisticalTest {
public:
    /**
     * @brief Execute the DFT Test
     * @param data Input bit sequence
     * @return Test result
     */
    TestResult execute(const BitSequence& data) override;
    
    /**
     * @brief Get test name
     * @return Test name
     */
    std::string getName() const override {
        return "Discrete Fourier Transform (Spectral) Test";
    }
};

/**
 * @class NonOverlappingTemplateTest
 * @brief NIST Non-overlapping Template Matching Test
 * 
 * Tests whether the frequency of non-overlapping occurrences of specific
 * bit patterns is as expected for a random sequence.
 */
class NonOverlappingTemplateTest : public StatisticalTest {
private:
    std::vector<std::vector<bool>> m_templates;
    size_t m_template_length;
    size_t m_block_size;
    
public:
    /**
     * @brief Constructor
     * @param template_length Length of templates to use (default 9)
     * @param block_size Size of blocks to analyze (default 8)
     */
    NonOverlappingTemplateTest(size_t template_length = 9, size_t block_size = 8);
    
    /**
     * @brief Execute the Non-overlapping Template Test
     * @param data Input bit sequence
     * @return Test result
     */
    TestResult execute(const BitSequence& data) override;
    
    /**
     * @brief Get test name
     * @return Test name
     */
    std::string getName() const override {
        return "Non-overlapping Template Matching Test";
    }
    
    /**
     * @brief Set custom templates
     * @param templates Vector of template bit patterns
     */
    void setTemplates(const std::vector<std::vector<bool>>& templates);
    
    /**
     * @brief Initialize standard templates
     */
    void initializeTemplates();
};

/**
 * @class OverlappingTemplateTest
 * @brief NIST Overlapping Template Matching Test
 * 
 * Tests whether the frequency of overlapping occurrences of specific
 * bit patterns is as expected for a random sequence.
 */
class OverlappingTemplateTest : public StatisticalTest {
private:
    std::vector<bool> m_template;
    size_t m_template_length;
    size_t m_block_size;
    
public:
    /**
     * @brief Constructor
     * @param template_bits Template bit pattern (default all ones)
     * @param block_size Size of blocks to analyze (default 1032)
     */
    OverlappingTemplateTest(
        const std::vector<bool>& template_bits = std::vector<bool>(9, true),
        size_t block_size = 1032
    );
    
    /**
     * @brief Execute the Overlapping Template Test
     * @param data Input bit sequence
     * @return Test result
     */
    TestResult execute(const BitSequence& data) override;
    
    /**
     * @brief Get test name
     * @return Test name
     */
    std::string getName() const override {
        return "Overlapping Template Matching Test";
    }
    
    /**
     * @brief Set template pattern
     * @param template_bits Template bit pattern
     */
    void setTemplate(const std::vector<bool>& template_bits);
};

} // namespace caca::tests::nist