// File: include/caca/tests/statistical_tests.hpp
#pragma once

#include "caca/core/bitsequence.hpp"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <cmath>

namespace caca::tests {

/**
 * @struct TestResult
 * @brief Structure to hold the result of a statistical test
 */
struct TestResult {
    std::string test_name;
    double p_value = 0.0;
    bool passed = false;
    std::map<std::string, double> additional_metrics;
};

/**
 * @class StatisticalTest
 * @brief Base class for all statistical tests
 * 
 * This abstract class defines the interface for statistical tests
 * used to evaluate the randomness of bit sequences.
 */
class StatisticalTest {
protected:
    // Default significance level (alpha)
    static constexpr double DEFAULT_ALPHA = 0.01;
    
    // Significance level
    double m_alpha = DEFAULT_ALPHA;
    
    // Common statistical functions
    static double erfc(double x) {
        return 1.0 - erf(x);
    }
    
    static double erf(double x) {
        // Constants for Abramowitz and Stegun approximation
        const double a1 = 0.254829592;
        const double a2 = -0.284496736;
        const double a3 = 1.421413741;
        const double a4 = -1.453152027;
        const double a5 = 1.061405429;
        const double p = 0.3275911;
        
        // Save the sign
        int sign = (x < 0) ? -1 : 1;
        x = std::fabs(x);
        
        // A&S formula 7.1.26
        double t = 1.0 / (1.0 + p * x);
        double y = 1.0 - ((((a5 * t + a4) * t + a3) * t + a2) * t + a1) * t * std::exp(-x * x);
        
        return sign * y;
    }
    
    // Check if p-value indicates passing test
    bool isPassed(double p_value) const {
        return p_value >= m_alpha;
    }

public:
    // Virtual destructor
    virtual ~StatisticalTest() = default;
    
    /**
     * @brief Execute the statistical test on bit sequence
     * @param data Input bit sequence
     * @return TestResult containing test results
     */
    virtual TestResult execute(const BitSequence& data) = 0;
    
    /**
     * @brief Execute the statistical test on byte sequence
     * @param data Input byte sequence
     * @return TestResult containing test results
     */
    virtual TestResult execute(const ByteSequence& data) {
        return execute(BitSequence(data));
    }
    
    /**
     * @brief Get the name of the test
     * @return Test name as string
     */
    virtual std::string getName() const = 0;
    
    /**
     * @brief Set the significance level (alpha)
     * @param alpha New significance level
     */
    void setAlpha(double alpha) {
        if (alpha <= 0.0 || alpha >= 1.0) {
            throw std::invalid_argument("Alpha must be between 0 and 1");
        }
        m_alpha = alpha;
    }
    
    /**
     * @brief Get current significance level
     * @return Current alpha value
     */
    double getAlpha() const {
        return m_alpha;
    }
};

/**
 * @class TestSuite
 * @brief Container for multiple statistical tests
 * 
 * This class manages a collection of statistical tests and provides
 * methods to run them all on a given data set.
 */
class TestSuite {
private:
    // Collection of statistical tests
    std::vector<std::unique_ptr<StatisticalTest>> m_tests;
    
    // Significance level for all tests
    double m_alpha = StatisticalTest::DEFAULT_ALPHA;

public:
    /**
     * @brief Default constructor
     */
    TestSuite() = default;
    
    /**
     * @brief Add a test to the suite
     * @param test Pointer to test instance (suite takes ownership)
     */
    void addTest(std::unique_ptr<StatisticalTest> test) {
        test->setAlpha(m_alpha);
        m_tests.push_back(std::move(test));
    }
    
    /**
     * @brief Set significance level for all tests
     * @param alpha New significance level
     */
    void setAlpha(double alpha) {
        if (alpha <= 0.0 || alpha >= 1.0) {
            throw std::invalid_argument("Alpha must be between 0 and 1");
        }
        m_alpha = alpha;
        
        // Update all tests
        for (auto& test : m_tests) {
            test->setAlpha(alpha);
        }
    }
    
    /**
     * @brief Run all tests on bit sequence
     * @param data Input bit sequence
     * @return Vector of test results
     */
    std::vector<TestResult> runTests(const BitSequence& data) {
        std::vector<TestResult> results;
        results.reserve(m_tests.size());
        
        for (auto& test : m_tests) {
            results.push_back(test->execute(data));
        }
        
        return results;
    }
    
    /**
     * @brief Run all tests on byte sequence
     * @param data Input byte sequence
     * @return Vector of test results
     */
    std::vector<TestResult> runTests(const ByteSequence& data) {
        std::vector<TestResult> results;
        results.reserve(m_tests.size());
        
        for (auto& test : m_tests) {
            results.push_back(test->execute(data));
        }
        
        return results;
    }
    
    /**
     * @brief Get all tests in the suite
     * @return Reference to the vector of tests
     */
    const std::vector<std::unique_ptr<StatisticalTest>>& getTests() const {
        return m_tests;
    }
    
    /**
     * @brief Clear all tests from the suite
     */
    void clear() {
        m_tests.clear();
    }
};

} // namespace caca::tests

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
    TestResult execute(const BitSequence& data) override {
        TestResult result;
        result.test_name = getName();
        
        // Ensure sufficient data length
        if (data.size() < 100) {
            result.p_value = 0.0;
            result.passed = false;
            result.additional_metrics["error"] = 1.0;
            return result;
        }
        
        // Count the number of ones
        size_t n = data.size();
        size_t ones_count = data.countOnes();
        
        // Compute the test statistic
        double sum = 2.0 * ones_count - n;
        double s_obs = std::fabs(sum) / std::sqrt(static_cast<double>(n));
        
        // Compute p-value
        double p_value = erfc(s_obs / std::sqrt(2.0));
        
        // Store results
        result.p_value = p_value;
        result.passed = isPassed(p_value);
        result.additional_metrics["ones_count"] = static_cast<double>(ones_count);
        result.additional_metrics["zeros_count"] = static_cast<double>(n - ones_count);
        result.additional_metrics["bias"] = static_cast<double>(ones_count) / n;
        
        return result;
    }
    
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
    TestResult execute(const BitSequence& data) override {
        TestResult result;
        result.test_name = getName();
        
        // Ensure sufficient data length
        if (data.size() < 100) {
            result.p_value = 0.0;
            result.passed = false;
            result.additional_metrics["error"] = 1.0;
            return result;
        }
        
        // Calculate number of complete blocks
        size_t n = data.size();
        size_t num_blocks = n / m_block_size;
        
        // Ensure at least 1 block
        if (num_blocks == 0) {
            result.p_value = 0.0;
            result.passed = false;
            result.additional_metrics["error"] = 2.0;
            return result;
        }
        
        // Process each block
        double chi_squared = 0.0;
        std::vector<double> block_proportions;
        
        for (size_t i = 0; i < num_blocks; ++i) {
            size_t block_start = i * m_block_size;
            size_t ones_count = 0;
            
            // Count ones in this block
            for (size_t j = 0; j < m_block_size; ++j) {
                if (data[block_start + j]) {
                    ones_count++;
                }
            }
            
            // Calculate proportion of ones in the block
            double pi = static_cast<double>(ones_count) / m_block_size;
            block_proportions.push_back(pi);
            
            // Add to chi-squared statistic
            double v = pi - 0.5;
            chi_squared += 4.0 * m_block_size * v * v;
        }
        
        // Compute p-value using chi-squared distribution
        double p_value = std::exp(-chi_squared / 2.0) * 
                          std::pow(chi_squared / 2.0, (num_blocks / 2.0) - 1.0) / 
                          std::tgamma(num_blocks / 2.0);
        
        // Store results
        result.p_value = p_value;
        result.passed = isPassed(p_value);
        result.additional_metrics["block_size"] = static_cast<double>(m_block_size);
        result.additional_metrics["num_blocks"] = static_cast<double>(num_blocks);
        result.additional_metrics["chi_squared"] = chi_squared;
        
        return result;
    }
    
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
    TestResult execute(const BitSequence& data) override {
        TestResult result;
        result.test_name = getName();
        
        // Ensure sufficient data length
        if (data.size() < 100) {
            result.p_value = 0.0;
            result.passed = false;
            result.additional_metrics["error"] = 1.0;
            return result;
        }
        
        // Count proportion of ones
        size_t n = data.size();
        size_t ones_count = data.countOnes();
        double pi = static_cast<double>(ones_count) / n;
        
        // Check if pi is suitable for this test
        if (std::fabs(pi - 0.5) >= (2.0 / std::sqrt(n))) {
            result.p_value = 0.0;
            result.passed = false;
            result.additional_metrics["error"] = 2.0;
            result.additional_metrics["pi"] = pi;
            return result;
        }
        
        // Count the number of runs
        size_t runs = 1;
        for (size_t i = 1; i < n; ++i) {
            if (data[i] != data[i - 1]) {
                runs++;
            }
        }
        
        // Compute test statistic
        double v_n_obs = static_cast<double>(runs);
        double expected_runs = 2.0 * n * pi * (1.0 - pi);
        double std_dev = std::sqrt(2.0 * n * pi * (1.0 - pi) * (1.0 - pi * (1.0 - pi)));
        double z = (v_n_obs - expected_runs) / std_dev;
        
        // Compute p-value
        double p_value = erfc(std::fabs(z) / std::sqrt(2.0));
        
        // Store results
        result.p_value = p_value;
        result.passed = isPassed(p_value);
        result.additional_metrics["pi"] = pi;
        result.additional_metrics["runs_count"] = static_cast<double>(runs);
        result.additional_metrics["expected_runs"] = expected_runs;
        result.additional_metrics["z_score"] = z;
        
        return result;
    }
    
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
    TestResult execute(const BitSequence& data) override {
        TestResult result;
        result.test_name = getName();
        
        // Ensure sufficient data length
        size_t n = data.size();
        if (n < 128) {
            result.p_value = 0.0;
            result.passed = false;
            result.additional_metrics["error"] = 1.0;
            return result;
        }
        
        // Determine parameters based on sequence length
        size_t M, K;
        const std::vector<size_t>* v_i;
        const std::vector<double>* pi_i;
        
        if (n < 6272) {
            M = 8;
            K = 3;
            v_i = &M_8;
            pi_i = &V_8;
        } else if (n < 750000) {
            M = 128;
            K = 5;
            v_i = &M_128;
            pi_i = &V_128;
        } else {
            M = 10000;
            K = 6;
            v_i = &M_10000;
            pi_i = &V_10000;
        }
        
        // Divide the sequence into blocks and find the longest run in each block
        size_t num_blocks = n / M;
        std::vector<size_t> frequencies(K + 1, 0);
        
        for (size_t i = 0; i < num_blocks; ++i) {
            size_t block_start = i * M;
            size_t longest_run = 0;
            size_t current_run = 0;
            
            for (size_t j = 0; j < M; ++j) {
                if (data[block_start + j]) {
                    current_run++;
                    longest_run = std::max(longest_run, current_run);
                } else {
                    current_run = 0;
                }
            }
            
            // Map longest run to the appropriate frequency bin
            size_t index = 0;
            for (size_t j = 0; j < K; ++j) {
                if (longest_run <= (*v_i)[j]) {
                    index = j;
                    break;
                }
            }
            
            if (index < K) {
                frequencies[index]++;
            } else {
                frequencies[K]++;
            }
        }
        
        // Compute chi-squared statistic
        double chi_squared = 0.0;
        for (size_t i = 0; i <= K; ++i) {
            double expected = num_blocks * (*pi_i)[i];
            double diff = frequencies[i] - expected;
            chi_squared += (diff * diff) / expected;
        }
        
        // Compute p-value using chi-squared distribution
        double p_value = std::exp(-chi_squared / 2.0) * 
                          std::pow(chi_squared / 2.0, (K / 2.0) - 1.0) / 
                          std::tgamma(K / 2.0);
        
        // Store results
        result.p_value = p_value;
        result.passed = isPassed(p_value);
        result.additional_metrics["block_size"] = static_cast<double>(M);
        result.additional_metrics["num_blocks"] = static_cast<double>(num_blocks);
        result.additional_metrics["chi_squared"] = chi_squared;
        
        return result;
    }
    
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
    TestResult execute(const BitSequence&
        data) override {
            TestResult result;
            result.test_name = getName();
            
            // Ensure sufficient data length
            size_t n = data.size();
            if (n < 1000) {
                result.p_value = 0.0;
                result.passed = false;
                result.additional_metrics["error"] = 1.0;
                return result;
            }
            
            // Convert binary data to +1/-1
            std::vector<double> X(n);
            for (size_t i = 0; i < n; ++i) {
                X[i] = data[i] ? 1.0 : -1.0;
            }
            
            // Apply DFT
            std::vector<std::complex<double>> S(n);
            for (size_t i = 0; i < n; ++i) {
                S[i] = 0.0;
                for (size_t j = 0; j < n; ++j) {
                    double angle = 2.0 * M_PI * i * j / n;
                    S[i] += X[j] * std::complex<double>(std::cos(angle), -std::sin(angle));
                }
            }
            
            // Find the modulus of each frequency component
            std::vector<double> modulus(n/2);
            for (size_t i = 0; i < n/2; ++i) {
                modulus[i] = std::abs(S[i]);
            }
            
            // Calculate threshold
            double T = std::sqrt(std::log(1.0 / 0.05) * n);
            
            // Count values below threshold
            size_t N0 = 0;
            for (size_t i = 0; i < n/2; ++i) {
                if (modulus[i] < T) {
                    N0++;
                }
            }
            
            // Calculate expected count
            size_t N1 = static_cast<size_t>(0.95 * n / 2);
            
            // Compute normalized difference
            double d = (N0 - N1) / std::sqrt(n * 0.95 * 0.05 / 4);
            
            // Compute p-value
            double p_value = erfc(std::abs(d) / std::sqrt(2.0));
            
            // Store results
            result.p_value = p_value;
            result.passed = isPassed(p_value);
            result.additional_metrics["threshold"] = T;
            result.additional_metrics["frequencies_below_threshold"] = static_cast<double>(N0);
            result.additional_metrics["expected_below_threshold"] = static_cast<double>(N1);
            result.additional_metrics["d_statistic"] = d;
            
            return result;
        }
        
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
        NonOverlappingTemplateTest(size_t template_length = 9, size_t block_size = 8)
            : m_template_length(template_length), m_block_size(block_size) {
            // Initialize with default templates
            initializeTemplates();
        }
        
        /**
         * @brief Initialize standard templates
         */
        void initializeTemplates() {
            // For simplicity, generate all possible templates of the given length
            // In practice, NIST recommends specific templates
            m_templates.clear();
            size_t num_templates = 1 << m_template_length;
            
            // Generate all possible bit patterns
            for (size_t i = 0; i < num_templates; ++i) {
                std::vector<bool> template_bits(m_template_length);
                for (size_t j = 0; j < m_template_length; ++j) {
                    template_bits[j] = (i >> j) & 1;
                }
                m_templates.push_back(template_bits);
            }
        }
        
        /**
         * @brief Execute the Non-overlapping Template Test
         * @param data Input bit sequence
         * @return Test result
         */
        TestResult execute(const BitSequence& data) override {
            TestResult result;
            result.test_name = getName();
            
            // Ensure sufficient data length
            size_t n = data.size();
            if (n < m_block_size * 100) {
                result.p_value = 0.0;
                result.passed = false;
                result.additional_metrics["error"] = 1.0;
                return result;
            }
            
            // Use only the first template for this implementation
            // In practice, all templates should be tested
            const std::vector<bool>& B = m_templates[0];
            
            // Calculate number of blocks
            size_t N = n / m_block_size;
            
            // Count occurrences in each block
            std::vector<size_t> W(N, 0);
            
            for (size_t i = 0; i < N; ++i) {
                size_t block_start = i * m_block_size;
                
                for (size_t j = 0; j <= m_block_size - m_template_length; j++) {
                    bool match = true;
                    
                    for (size_t k = 0; k < m_template_length && match; ++k) {
                        if (data[block_start + j + k] != B[k]) {
                            match = false;
                        }
                    }
                    
                    if (match) {
                        W[i]++;
                        j += m_template_length - 1;
                    }
                }
            }
            
            // Calculate mean and variance
            double mu = (m_block_size - m_template_length + 1) / std::pow(2.0, m_template_length);
            double sigma_squared = m_block_size * (1.0 / std::pow(2.0, m_template_length)) * 
                                  (1.0 - 1.0 / std::pow(2.0, m_template_length));
            
            // Calculate chi-squared statistic
            double chi_squared = 0.0;
            for (size_t i = 0; i < N; ++i) {
                double diff = W[i] - mu;
                chi_squared += (diff * diff) / sigma_squared;
            }
            
            // Compute p-value
            double p_value = std::exp(-chi_squared / 2.0) * 
                              std::pow(chi_squared / 2.0, (N / 2.0) - 1.0) / 
                              std::tgamma(N / 2.0);
            
            // Store results
            result.p_value = p_value;
            result.passed = isPassed(p_value);
            result.additional_metrics["template_length"] = static_cast<double>(m_template_length);
            result.additional_metrics["block_size"] = static_cast<double>(m_block_size);
            result.additional_metrics["num_blocks"] = static_cast<double>(N);
            result.additional_metrics["expected_matches_per_block"] = mu;
            result.additional_metrics["chi_squared"] = chi_squared;
            
            return result;
        }
        
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
        void setTemplates(const std::vector<std::vector<bool>>& templates) {
            if (templates.empty()) {
                throw std::invalid_argument("Templates cannot be empty");
            }
            
            size_t template_length = templates[0].size();
            for (const auto& templ : templates) {
                if (templ.size() != template_length) {
                    throw std::invalid_argument("All templates must have the same length");
                }
            }
            
            m_templates = templates;
            m_template_length = template_length;
        }
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
        OverlappingTemplateTest(const std::vector<bool>& template_bits = std::vector<bool>(9, true),
                                size_t block_size = 1032)
            : m_template(template_bits), m_template_length(template_bits.size()), 
              m_block_size(block_size) {}
        
        /**
         * @brief Execute the Overlapping Template Test
         * @param data Input bit sequence
         * @return Test result
         */
        TestResult execute(const BitSequence& data) override {
            TestResult result;
            result.test_name = getName();
            
            // Ensure sufficient data length
            size_t n = data.size();
            if (n < m_block_size * 100) {
                result.p_value = 0.0;
                result.passed = false;
                result.additional_metrics["error"] = 1.0;
                return result;
            }
            
            // Calculate number of blocks
            size_t N = n / m_block_size;
            
            // Calculate parameters
            size_t m = m_template_length;
            size_t M = m_block_size;
            double lambda = (M - m + 1) / std::pow(2.0, m);
            double eta = lambda / 2.0;
            
            // Define frequency categories (0, 1, 2, 3, 4, 5+)
            size_t K = 5;
            std::vector<double> pi(K + 1);
            
            // Calculate category probabilities
            pi[0] = std::exp(-eta);
            pi[1] = eta * std::exp(-eta);
            pi[2] = (eta * eta / 2.0) * std::exp(-eta);
            pi[3] = (eta * eta * eta / 6.0) * std::exp(-eta);
            pi[4] = (eta * eta * eta * eta / 24.0) * std::exp(-eta);
            pi[5] = 1.0 - (pi[0] + pi[1] + pi[2] + pi[3] + pi[4]);
            
            // Count occurrences in each block
            std::vector<size_t> v(K + 1, 0);
            
            for (size_t i = 0; i < N; ++i) {
                size_t block_start = i * m_block_size;
                
                // Count matches in this block
                size_t matches = 0;
                for (size_t j = 0; j <= M - m; ++j) {
                    bool match = true;
                    
                    for (size_t k = 0; k < m && match; ++k) {
                        if (data[block_start + j + k] != m_template[k]) {
                            match = false;
                        }
                    }
                    
                    if (match) {
                        matches++;
                    }
                }
                
                // Map matches to the appropriate category
                if (matches > K) {
                    v[K]++;
                } else {
                    v[matches]++;
                }
            }
            
            // Calculate chi-squared statistic
            double chi_squared = 0.0;
            for (size_t i = 0; i <= K; ++i) {
                double expected = N * pi[i];
                double diff = v[i] - expected;
                chi_squared += (diff * diff) / expected;
            }
            
            // Compute p-value
            double p_value = std::exp(-chi_squared / 2.0) * 
                              std::pow(chi_squared / 2.0, (K / 2.0) - 1.0) / 
                              std::tgamma(K / 2.0);
            
            // Store results
            result.p_value = p_value;
            result.passed = isPassed(p_value);
            result.additional_metrics["template_length"] = static_cast<double>(m_template_length);
            result.additional_metrics["block_size"] = static_cast<double>(m_block_size);
            result.additional_metrics["num_blocks"] = static_cast<double>(N);
            result.additional_metrics["lambda"] = lambda;
            result.additional_metrics["chi_squared"] = chi_squared;
            
            return result;
        }
        
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
        void setTemplate(const std::vector<bool>& template_bits) {
            if (template_bits.empty()) {
                throw std::invalid_argument("Template cannot be empty");
            }
            
            m_template = template_bits;
            m_template_length = template_bits.size();
        }
    };
    
    // Static member initialization for LongestRunTest
    const std::vector<size_t> LongestRunTest::M_8 = {1, 2, 3};
    const std::vector<size_t> LongestRunTest::M_128 = {4, 5, 6, 7, 8};
    const std::vector<size_t> LongestRunTest::M_10000 = {10, 11, 12, 13, 14, 15};
    
    const std::vector<double> LongestRunTest::V_8 = {0.21484375, 0.3671875, 0.23046875, 0.1875};
    const std::vector<double> LongestRunTest::V_128 = {0.1174, 0.2430, 0.2493, 0.1752, 0.1027, 0.1124};
    const std::vector<double> LongestRunTest::V_10000 = {0.0882, 0.2092, 0.2483, 0.1933, 0.1208, 0.0675, 0.0727};
    
    } // namespace caca::tests::nist
    
    // File: src/tests/nist_tests.cpp
    #include "caca/tests/nist_tests.hpp"
    
    namespace caca::tests::nist {
        // Implementation of any non-template methods would go here
        // Currently all methods are fully implemented in the headers
    }