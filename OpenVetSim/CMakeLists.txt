cmake_minimum_required(VERSION 3.20)
project(WinVetSim VERSION 2.4 LANGUAGES CXX)

# ----------------------------------------------------------------
# C++ standard — C++17 required for std::filesystem
# ----------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----------------------------------------------------------------
# Source files
# All simulation-logic files are shared between all platforms.
# Platform-specific files are gated by #ifdef in each .cpp / .h.
# ----------------------------------------------------------------
set(SOURCES
    main.cpp
    VetSim.cpp
    WebSrv.cpp
    XMLRead.cpp
    bcastServer.cpp
    cgiClass.cpp
    keys.cpp
    llist.cpp
    pulse.cpp
    queryInfo.cpp
    scenario.cpp
    scenario_xml.cpp
    sim-parse.cpp
    simlog.cpp
    simmgrCommon.cpp
    simmgrVideo.cpp
    simstatus.cpp
    simutil.cpp
    soundInit.cpp
    vetsimTasks.cpp
    strptime.cpp
    Source.cpp
)

# sendKeys.cpp contains Windows-only keyboard automation; stub on POSIX
if(WIN32)
    list(APPEND SOURCES sendKeys.cpp)
endif()

# ----------------------------------------------------------------
# Executable
# ----------------------------------------------------------------
add_executable(${PROJECT_NAME} ${SOURCES})

# ----------------------------------------------------------------
# Include directories
# ----------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/args
)

# ----------------------------------------------------------------
# Compiler options
# ----------------------------------------------------------------
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4
        /EHsc
        /MP
        /utf-8
    >
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
        -Wall
        -Wextra
        -Wno-unused-parameter
        -pthread
    >
)

# ----------------------------------------------------------------
# Preprocessor definitions
# ----------------------------------------------------------------
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        _UNICODE
        UNICODE
        DPSAPI_VERSION=1
        _CRT_SECURE_NO_WARNINGS     # suppress MSVC safe-function warnings during migration
    >
    $<$<CONFIG:Debug>:DEBUG>
)

# ----------------------------------------------------------------
# Platform-specific link libraries
# ----------------------------------------------------------------
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Ws2_32      # WinSock2
        Psapi       # Process status API
        winmm       # Windows Multimedia (timing)
    )
elseif(APPLE)
    # macOS: pthreads are part of libSystem; filesystem is in libc++
    target_link_libraries(${PROJECT_NAME} PRIVATE
        pthread
    )
    # Ensure std::filesystem links correctly on older Xcode toolchains
    target_compile_options(${PROJECT_NAME} PRIVATE -mmacosx-version-min=10.15)
    target_link_options(${PROJECT_NAME} PRIVATE -mmacosx-version-min=10.15)
else()
    # Linux
    target_link_libraries(${PROJECT_NAME} PRIVATE
        pthread
        stdc++fs    # GCC < 9 needs explicit filesystem lib (no-op on GCC 9+)
    )
endif()

# ----------------------------------------------------------------
# nlohmann/json
#
# The NuGet package is Windows-only. On POSIX we use the bundled
# single-header version if present, or let CMake fetch it.
# ----------------------------------------------------------------
if(WIN32)
    # NuGet places headers under packages/nlohmann.json.x.y.z/build/native/include
    # Adjust the path below to match your NuGet cache location, or use vcpkg.
    find_path(NLOHMANN_JSON_INCLUDE
        NAMES nlohmann/json.hpp
        HINTS
            "$ENV{USERPROFILE}/.nuget/packages/nlohmann.json.3.12.0/build/native/include"
            "${CMAKE_SOURCE_DIR}/packages/nlohmann.json.3.12.0/build/native/include"
    )
    if(NLOHMANN_JSON_INCLUDE)
        target_include_directories(${PROJECT_NAME} PRIVATE ${NLOHMANN_JSON_INCLUDE})
    else()
        message(WARNING "nlohmann/json not found via NuGet; trying system or FetchContent")
    endif()
endif()

# If not found on Windows, or on POSIX, try FetchContent as a fallback
if(NOT WIN32 OR NOT NLOHMANN_JSON_INCLUDE)
    find_package(nlohmann_json QUIET)
    if(nlohmann_json_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
    else()
        include(FetchContent)
        FetchContent_Declare(
            nlohmann_json
            URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp
            DOWNLOAD_NO_EXTRACT TRUE
            DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/nlohmann"
        )
        FetchContent_MakeAvailable(nlohmann_json)
        target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_BINARY_DIR}/nlohmann")
        message(STATUS "nlohmann/json: using FetchContent (single header)")
    endif()
endif()

# ----------------------------------------------------------------
# Output directories — keep binaries tidy
# ----------------------------------------------------------------
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ----------------------------------------------------------------
# Install rules (optional)
# ----------------------------------------------------------------
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# ----------------------------------------------------------------
# Summary
# ----------------------------------------------------------------
message(STATUS "")
message(STATUS "=== WinVetSim / OpenVetSim CMake Configuration ===")
message(STATUS "  Platform   : ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler   : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ std    : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "==================================================")
message(STATUS "")
