// File: src/tests/statistical_tests.cpp
#include "caca/tests/statistical_tests.hpp"
#include "caca/core/error_handling.hpp"
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace caca::tests {

// Implementation of any non-inline methods from the statistical_tests.hpp file

// For now, most of the functionality is implemented as inline methods in the header,
// but this file serves as the implementation file for any additional methods
// that might be added later.

// Example implementations that could be added:

// Utility functions for statistical calculations
namespace util {
    // Calculate mean of a vector
    double mean(const std::vector<double>& values) {
        if (values.empty()) {
            return 0.0;
        }
        
        return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    }
    
    // Calculate variance of a vector
    double variance(const std::vector<double>& values) {
        if (values.size() < 2) {
            return 0.0;
        }
        
        double m = mean(values);
        double sum_squared_diff = 0.0;
        
        for (double value : values) {
            double diff = value - m;
            sum_squared_diff += diff * diff;
        }
        
        return sum_squared_diff / (values.size() - 1);
    }
    
    // Calculate standard deviation of a vector
    double standard_deviation(const std::vector<double>& values) {
        return std::sqrt(variance(values));
    }
    
    // Chi-square test for goodness of fit
    double chi_square_test(const std::vector<double>& observed, const std::vector<double>& expected) {
        if (observed.size() != expected.size()) {
            throw std::invalid_argument("Observed and expected vectors must have the same size");
        }
        
        double chi_square = 0.0;
        
        for (size_t i = 0; i < observed.size(); ++i) {
            if (expected[i] <= 0.0) {
                throw std::invalid_argument("Expected values must be positive");
            }
            
            double diff = observed[i] - expected[i];
            chi_square += (diff * diff) / expected[i];
        }
        
        return chi_square;
    }
    
    // Calculate p-value from chi-square statistic
    double p_value_from_chi_square(double chi_square, size_t degrees_of_freedom) {
        // This is a simplified approximation for calculating p-values
        // For precise values, a more comprehensive statistical library should be used
        
        // For small degrees of freedom, use lookup tables or approximations
        if (degrees_of_freedom <= 1) {
            return std::exp(-chi_square / 2.0);
        }
        
        // For larger degrees of freedom, use normal approximation
        double z = std::sqrt(2.0 * chi_square) - std::sqrt(2.0 * degrees_of_freedom - 1.0);
        return 0.5 * StatisticalTest::erfc(z / std::sqrt(2.0));
    }
}

} // namespace caca::tests