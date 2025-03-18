# CACA: Cellular Automata CryptAnalysis

## Authors Note

I'd like to note this project is a perfect example of how absolutely crucial cross-disciplinary interest/education and interactions are - if i jhadn't happened to read a medical journal paper on pure 'oh that looks odd and interesting' offchance then this would never have been!

## Overview

CACA is an advanced tool for cryptographic analysis using Cellular Automata and statistical testing techniques. The project aims to provide deep insights into the randomness and potential weaknesses of encrypted data without decryption.

## Features

- NIST Statistical Test Suite Implementation
- Cellular Automata-based Pattern Detection
- SIMD-Optimized Performance
- Comprehensive Cryptographic Data Analysis

## Prerequisites

- C++20 Compiler (GCC 10+, Clang 10+, MSVC 19.25+)
- CMake 3.20+
- argparse library
- Optional: OpenMP for parallel processing

## Building the Project

```bash
# Clone the repository
git clone https://github.com/yourusername/caca.git
cd caca

# Create build directory
mkdir build && cd build

# Configure the project
cmake ..

# Build
make
```

## Usage

```bash
# Basic usage
./caca_app input_file.bin

# Verbose logging
./caca_app -v input_file.bin
```

## Supported Platforms

- Linux
- macOS
- Windows

## Planned Features

- More advanced NIST Statistical Tests
- Enhanced Cellular Automata Analysis
- Expanded SIMD Optimization
- Machine Learning Integration for Pattern Recognition

## Contributing

Contributions are welcome! Please read our CONTRIBUTING.md for details on our code of conduct and the process for submitting pull requests.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgements

- 	NIST Statistical Test Suite
- 	Cellular Automata Research Community
- 	AnthropicAI and OpenAI without whom (Claude 3.7 Sonnet, 		Claude 3.5 Sonnet, Claude 3.5 Haiku, Claude 3 Opus, 			chatGPT4.5preview, chatGPTo1, chatGPT4o and chatGPTo1-	mini-High) the code never would've even been done let 	alone to such astonishing quality and in such record 	time!
- 	Emily - she's the only one who didn't give up on me - no 	matter what.
- 	Suresh, A. et. al. who originally turned mme onto using 		CAs to alter noise profiles hence inspiring this whole 			project - thankyou from the bottom of my heart :)