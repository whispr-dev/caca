#pragma once

#include "statistical_tests.hpp"

namespace caca::tests::nist {
    // Frequency (Monobit) Test
    class FrequencyTest : public StatisticalTest {
    public:
        bool execute(const ByteSequence& data) override {
            // Convert bytes to bit sequence
            std::vector<bool> bits;
            for (uint8_t byte : data) {
                for (int i = 7; i >= 0; --i) {
                    bits.push_back((byte >> i) & 1);
                }
            }

            // Calculate proportion of 1s
            size_t ones_count = std::count(bits.begin(), bits.end(), true);
            double proportion = static_cast<double>(ones_count) / bits.size();

            // Calculate test statistic
            double s_obs = std::abs(proportion - 0.5) * std::sqrt(bits.size() * 2.0);
            
            // Calculate p-value
            double p_value = statistical_utils::erfc(s_obs / std::sqrt(2.0));

            // Typical significance level is 0.01
            return p_value >= 0.01;
        }

        std::string getName() const override {
            return "Frequency (Monobit) Test";
        }
    };

    // Runs Test
    class RunsTest : public StatisticalTest {
    public:
        bool execute(const ByteSequence& data) override {
            // Convert bytes to bit sequence
            std::vector<bool> bits;
            for (uint8_t byte : data) {
                for (int i = 7; i >= 0; --i) {
                    bits.push_back((byte >> i) & 1);
                }
            }

            // Calculate proportion of 1s
            size_t ones_count = std::count(bits.begin(), bits.end(), true);
            double pi = static_cast<double>(ones_count) / bits.size();

            // Check if proportion is suitable for runs test
            if (std::abs(pi - 0.5) >= (2.0 / std::sqrt(bits.size()))) {
                return false;
            }

            // Count runs
            size_t runs = 1;
            for (size_t i = 1; i < bits.size(); ++i) {
                if (bits[i] != bits[i-1]) {
                    ++runs;
                }
            }

            // Calculate test statistic
            double r_obs = runs;
            double mean = 2.0 * bits.size() * pi * (1.0 - pi);
            double std_dev = std::sqrt(2.0 * bits.size() * pi * (1.0 - pi));
            
            double z = std::abs(r_obs - mean) / std_dev;
            double p_value = statistical_utils::erfc(z / std::sqrt(2.0));

            // Typical significance level is 0.01
            return p_value >= 0.01;
        }

        std::string getName() const override {
            return "Runs Test";
        }
    };

    // Block Frequency Test
    class BlockFrequencyTest : public StatisticalTest {
    private:
        size_t block_length = 128;  // Default block length

    public:
        void configure(const std::vector<double>& params) override {
            if (!params.empty()) {
                block_length = static_cast<size_t>(params[0]);
            }
        }

        bool execute(const ByteSequence& data) override {
            // Convert bytes to bit sequence
            std::vector<bool> bits;
            for (uint8_t byte : data) {
                for (int i = 7; i >= 0; --i) {
                    bits.push_back((byte >> i) & 1);
                }
            }

            // Calculate number of complete blocks
            size_t num_blocks = bits.size() / block_length;
            if (num_blocks == 0) {
                return false;
            }

            // Compute chi-squared statistic
            double chi_squared = 0.0;
            for (size_t i = 0; i < num_blocks; ++i) {
                // Count 1s in this block
                size_t block_ones = 0;
                for (size_t j = 0; j < block_length; ++j) {
                    if (bits[i * block_length + j]) {
                        ++block_ones;
                    }
                }

                // Proportion of 1s in the block
                double pi = static_cast<double>(block_ones) / block_length;
                double v = pi - 0.5;
                chi_squared += 4.0 * block_length * v * v;
            }

            // Calculate p-value
            double p_value = std::exp(-chi_squared / 2.0);

            // Typical significance level is 0.01
            return p_value >= 0.01;
        }

        std::string getName() const override {
            return "Block Frequency Test";
        }
    };
}