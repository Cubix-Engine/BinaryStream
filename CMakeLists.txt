cmake_minimum_required(VERSION 3.28)

project(BinaryStream LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Useful options for local dev and CI
option(BINARYSTREAM_BUILD_EXAMPLES "Build examples" ON)
option(ENABLE_WARNINGS "Enable warnings" ON)
option(WARNINGS_AS_ERRORS "Treat warnings as errors" ON)


# Library
add_library(BinaryStream INTERFACE)
add_library(BinaryStream::BinaryStream ALIAS BinaryStream)

target_compile_features(BinaryStream INTERFACE cxx_std_23)
target_compile_options(BinaryStream INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /Zc:preprocessor>
)

target_compile_definitions(BinaryStream INTERFACE
    $<$<PLATFORM_ID:Windows>:NOMINMAX WIN32_LEAN_AND_MEAN>
    $<$<CONFIG:Debug>:BINARYSTREAM_DEBUG=1>
)

target_include_directories(BinaryStream INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# Compiler warnings
if (PROJECT_IS_TOP_LEVEL AND ENABLE_WARNINGS)
    include(cmake/CompilerWarnings.cmake)
    add_library(BinaryStream_warnings INTERFACE)
    set_project_warnings(BinaryStream_warnings ${WARNINGS_AS_ERRORS})
endif()


# Examples
if (BINARYSTREAM_BUILD_EXAMPLES)
    add_subdirectory(src-example)
endif()


# Install
install(TARGETS BinaryStream
    EXPORT BinaryStreamTargets
)

install(DIRECTORY src/BinaryStream/
    DESTINATION include/BinaryStream
)

install(EXPORT BinaryStreamTargets
    FILE BinaryStreamTargets.cmake
    NAMESPACE BinaryStream::
    DESTINATION lib/cmake/BinaryStream
)

# Tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)