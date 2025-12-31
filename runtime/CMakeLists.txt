cmake_minimum_required(VERSION 3.20)

project(CoreMLWinRuntime VERSION 0.1.0 LANGUAGES C CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Options
option(CMW_BUILD_TESTS "Build tests" ON)
option(CMW_BUILD_PROVIDERS "Build built-in providers" ON)
option(CMW_ENABLE_TELEMETRY "Enable telemetry" OFF)

# Platform checks
if(NOT WIN32)
    message(WARNING "CoreMLWin is designed for Windows, but attempting to build anyway")
endif()

# Find dependencies
find_package(Protobuf REQUIRED)
find_package(onnxruntime CONFIG)

if(NOT onnxruntime_FOUND)
    message(STATUS "ONNX Runtime not found via CMake, will search manually")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/generated  # For protobuf generated files
    ${Protobuf_INCLUDE_DIRS}
)

# ============================================================================
# Protobuf Generation
# ============================================================================

set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../protos)
set(PROTO_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/generated/coremlwin_runtime.pb.cc
)
set(PROTO_HDRS
    ${CMAKE_CURRENT_BINARY_DIR}/generated/coremlwin_runtime.pb.h
)

add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/generated
    COMMAND protoc
        --proto_path=${PROTO_DIR}
        --cpp_out=${CMAKE_CURRENT_BINARY_DIR}/generated
        ${PROTO_DIR}/coremlwin_runtime.proto
    DEPENDS ${PROTO_DIR}/coremlwin_runtime.proto
    COMMENT "Generating protobuf C++ sources"
)

# ============================================================================
# Core Runtime Library
# ============================================================================

add_library(coremlwin_core STATIC
    # Error handling
    service/coremlwin_errors.c

    # Protobuf generated
    ${PROTO_SRCS}

    # Headers
    include/coremlwin_errors.h
    include/coremlwin_provider_api.h
)

target_link_libraries(coremlwin_core
    PUBLIC
        ${Protobuf_LIBRARIES}
)

target_include_directories(coremlwin_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/generated
)

# ============================================================================
# Runtime Service Executable
# ============================================================================

# Runtime service sources
set(SERVICE_SOURCES
    service/main.cpp
    service/model_registry.cpp
    service/runtime_state.cpp
    execution/onnx_executor.cpp
)

# Windows-specific sources
if(WIN32)
    list(APPEND SERVICE_SOURCES
        service/named_pipe_server_win32.cpp
    )
endif()

add_executable(coremlwin_service
    ${SERVICE_SOURCES}
)

target_link_libraries(coremlwin_service
    PRIVATE
        coremlwin_core
)

if(WIN32)
    # Windows-specific libraries
    target_link_libraries(coremlwin_service
        PRIVATE
            ws2_32
            # Additional Windows libraries as needed
    )
endif()

# ============================================================================
# Provider Plugins
# ============================================================================

if(CMW_BUILD_PROVIDERS)
    # CPU Provider (ONNX Runtime)
    if(onnxruntime_FOUND OR EXISTS "${ONNXRUNTIME_DIR}")
        add_library(coremlwin_provider_cpu SHARED
            # providers/cpu/cpu_provider.cpp     # To be implemented
        )

        target_link_libraries(coremlwin_provider_cpu
            PRIVATE
                coremlwin_core
                # onnxruntime
        )

        # Set provider plugin naming convention
        set_target_properties(coremlwin_provider_cpu PROPERTIES
            PREFIX ""
            OUTPUT_NAME "coremlwin_provider_cpu"
        )
    endif()

    # DirectML Provider
    if(WIN32)
        add_library(coremlwin_provider_directml SHARED
            # providers/directml/directml_provider.cpp  # To be implemented
        )

        target_link_libraries(coremlwin_provider_directml
            PRIVATE
                coremlwin_core
                # DirectML (find or vendored)
        )

        set_target_properties(coremlwin_provider_directml PROPERTIES
            PREFIX ""
            OUTPUT_NAME "coremlwin_provider_directml"
        )
    endif()
endif()

# ============================================================================
# Installation
# ============================================================================

install(TARGETS coremlwin_service
    RUNTIME DESTINATION bin
)

install(TARGETS coremlwin_core
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include/coremlwin
    FILES_MATCHING PATTERN "*.h"
)

if(CMW_BUILD_PROVIDERS)
    install(TARGETS coremlwin_provider_cpu coremlwin_provider_directml
        LIBRARY DESTINATION providers
        RUNTIME DESTINATION providers
    )
endif()

# Install configs
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../configs/
    DESTINATION etc/coremlwin
)

# ============================================================================
# Tests
# ============================================================================

if(CMW_BUILD_TESTS)
    enable_testing()
    # add_subdirectory(tests)  # To be implemented
endif()

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "CoreMLWin Runtime Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build tests: ${CMW_BUILD_TESTS}")
message(STATUS "  Build providers: ${CMW_BUILD_PROVIDERS}")
message(STATUS "  Protobuf: ${Protobuf_VERSION}")
message(STATUS "")
