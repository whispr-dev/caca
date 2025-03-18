# Specify the minimum CMake version required
cmake_minimum_required(VERSION 3.20)

# Define the project with its name, version, and languages
project(CACA 
    VERSION 1.0
    DESCRIPTION "Cellular Automata CryptAnalysis"
    LANGUAGES CXX
)

# Modifications for Windows compatibility

# Prefer using Windows native threading
if(WIN32)
    set(THREADS_PREFER_PTHREAD_FLAG OFF)
endif()

# Find packages more flexibly
find_package(Threads REQUIRED)
# Disable OpenMP if not supported
if(ENABLE_OPENMP)
    find_package(OpenMP QUIET)
    if(NOT OpenMP_CXX_FOUND)
        message(WARNING "OpenMP not found. Disabling OpenMP support.")
        set(ENABLE_OPENMP OFF)
    endif()
endif()

# Optional JSON library with more flexible finding
if(USE_JSON)
    find_package(nlohmann_json CONFIG QUIET)
    if(NOT nlohmann_json_FOUND)
        message(WARNING "nlohmann_json not found. Attempting to fetch...")
        include(FetchContent)
        FetchContent_Declare(
            json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG v3.11.2
        )
        FetchContent_MakeAvailable(json)
    endif()
endif()

# Adjust SIMD flags for MSVC to be more robust
if(MSVC)
    if(ENABLE_SIMD)
        # Check for AVX support more comprehensively
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag("/arch:AVX2" AVX2_SUPPORTED)
        check_cxx_compiler_flag("/arch:AVX" AVX_SUPPORTED)
        
        if(AVX2_SUPPORTED)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
        elseif(AVX_SUPPORTED)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX")
        endif()
    endif()
endif()