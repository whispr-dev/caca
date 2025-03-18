#include <iostream>
#include <fstream>
#include <vector>
#include <argparse/argparse.hpp>

int main(int argc, char* argv[]) {
    // Create an argument parser
    argparse::ArgumentParser program("caca");

    // Add an input file argument
    program.add_argument("input_file")
        .help("Path to the input binary file to analyze")
        .required();

    try {
        // Parse arguments
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    // Get the input file path
    std::string input_file_path = program.get<std::string>("input_file");

    // Open the input file
    std::ifstream input_file(input_file_path, std::ios::binary);
    
    // Check if file opened successfully
    if (!input_file) {
        std::cerr << "Error: Could not open input file " << input_file_path << std::endl;
        return 1;
    }

    // Read the file contents
    std::vector<char> buffer(
        (std::istreambuf_iterator<char>(input_file)), 
        std::istreambuf_iterator<char>()
    );

    // Print basic file information
    std::cout << "Input File: " << input_file_path << std::endl;
    std::cout << "File Size: " << buffer.size() << " bytes" << std::endl;

    // Basic sanity check
    if (buffer.empty()) {
        std::cerr << "Warning: Input file is empty" << std::endl;
        return 1;
    }

    // TODO: Add actual statistical tests here
    std::cout << "File read successfully!" << std::endl;

    return 0;
}