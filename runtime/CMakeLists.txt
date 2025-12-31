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

# Find ONNX Runtime
if(DEFINED ENV{ONNXRUNTIME_DIR})
    set(ONNXRUNTIME_DIR $ENV{ONNXRUNTIME_DIR})
    message(STATUS "Using ONNX Runtime from: ${ONNXRUNTIME_DIR}")

    # Verify installation
    if(EXISTS "${ONNXRUNTIME_DIR}/include/onnxruntime_cxx_api.h")
        set(onnxruntime_FOUND TRUE)
        set(ONNXRUNTIME_INCLUDE_DIRS "${ONNXRUNTIME_DIR}/include")

        # Find library file
        if(WIN32)
            set(ONNXRUNTIME_LIBRARIES "${ONNXRUNTIME_DIR}/lib/onnxruntime.lib")
            set(ONNXRUNTIME_DLL "${ONNXRUNTIME_DIR}/lib/onnxruntime.dll")
        else()
            set(ONNXRUNTIME_LIBRARIES "${ONNXRUNTIME_DIR}/lib/libonnxruntime.so")
        endif()

        message(STATUS "  Include: ${ONNXRUNTIME_INCLUDE_DIRS}")
        message(STATUS "  Library: ${ONNXRUNTIME_LIBRARIES}")
    else()
        message(WARNING "ONNXRUNTIME_DIR set but installation not found at: ${ONNXRUNTIME_DIR}")
        set(onnxruntime_FOUND FALSE)
    endif()
else()
    message(STATUS "ONNXRUNTIME_DIR not set, trying CMake find_package...")
    find_package(onnxruntime CONFIG)

    if(onnxruntime_FOUND)
        message(STATUS "Found ONNX Runtime via CMake")
    endif()
endif()

if(NOT onnxruntime_FOUND)
    message(WARNING "ONNX Runtime not found. Building with placeholder implementation.")
    message(WARNING "Set ONNXRUNTIME_DIR environment variable to enable real inference.")
    message(WARNING "See docs/ONNX_RUNTIME_SETUP.md for setup instructions.")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/generated  # For protobuf generated files
    ${Protobuf_INCLUDE_DIRS}
)

# Add ONNX Runtime includes if found
if(onnxruntime_FOUND)
    include_directories(${ONNXRUNTIME_INCLUDE_DIRS})
    add_compile_definitions(CMW_HAVE_ONNXRUNTIME)
endif()

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
    service/logger.cpp
    service/runtime_config.cpp
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

# Link ONNX Runtime if available
if(onnxruntime_FOUND)
    target_link_libraries(coremlwin_service
        PRIVATE
            ${ONNXRUNTIME_LIBRARIES}
    )

    # Copy DLL to output directory on Windows
    if(WIN32 AND EXISTS "${ONNXRUNTIME_DLL}")
        add_custom_command(TARGET coremlwin_service POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${ONNXRUNTIME_DLL}"
                "$<TARGET_FILE_DIR:coremlwin_service>"
            COMMENT "Copying onnxruntime.dll to output directory"
        )

        # Also copy DirectML.dll if it exists
        if(EXISTS "${ONNXRUNTIME_DIR}/lib/DirectML.dll")
            add_custom_command(TARGET coremlwin_service POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${ONNXRUNTIME_DIR}/lib/DirectML.dll"
                    "$<TARGET_FILE_DIR:coremlwin_service>"
                COMMENT "Copying DirectML.dll to output directory"
            )
        endif()
    endif()
endif()

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

if(onnxruntime_FOUND)
    message(STATUS "  ONNX Runtime: ENABLED")
    message(STATUS "    Location: ${ONNXRUNTIME_DIR}")
else()
    message(STATUS "  ONNX Runtime: DISABLED (using placeholder)")
    message(STATUS "    Set ONNXRUNTIME_DIR to enable real inference")
endif()

message(STATUS "")
