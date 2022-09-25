cmake_minimum_required(VERSION 3.7)
project(grunka VERSION 1.0.0)

if (NOT MSVC)
    message(FATAL_ERROR "Unsupported platform")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(LIVE_PP False)
set(SUPERLUMINAL False)

# Find locally installed dependencies. Tip: Use VCPKG for these.

if (SUPERLUMINAL)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "c:/Program Files/Superluminal/Performance/API")
    find_package(SuperluminalAPI REQUIRED)
endif()


# Fetch dependencies from Github

include(FetchContent)

FetchContent_Declare(
    BitSquidFoundation
    GIT_REPOSITORY https://github.com/hsjunnesson/bitsquid-foundation.git
)

if (NOT bitsquidfoundation_POPULATED)
    FetchContent_MakeAvailable(BitSquidFoundation)
endif()

# WWISE
set(WWISE_SDK "c:/Program Files (x86)/Audiokinetic/Wwise 2021.1.10.7883/SDK")
find_package(Wwise REQUIRED)

add_subdirectory("${CMAKE_SOURCE_DIR}/chocolate")



# Main game source

file(GLOB_RECURSE SRC_grunka
    "src/*.cpp"
    "src/*.h"
    "src/*.c"
    "src/*.hpp"
    "src/*.inl"
)

# Wwise sources
file(GLOB_RECURSE SRC_AK
    "AK/*.h"
    "AK/*.cpp"
)


# Create executable
add_executable(${PROJECT_NAME}
    ${SRC_grunka}
    ${SRC_AK}
)


# Includes

include_directories(SYSTEM ${backward_SOURCE_DIR})
include_directories(SYSTEM ${Wwise_INCLUDE_DIR})
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/AK)
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/AK/Win32)

if (SUPERLUMINAL)
    include_directories(SYSTEM ${SuperluminalAPI_INCLUDE_DIRS})
endif()

if (LIVE_PP)
    include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/LivePP/API)
endif()


# Linked libraries

target_link_libraries(${PROJECT_NAME} PRIVATE chocolate)

if (SUPERLUMINAL)
    target_link_libraries(${PROJECT_NAME} PRIVATE SuperluminalAPI)
endif()

foreach(lib ${Wwise_LIBRARIES})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${lib})
endforeach()

target_link_libraries(${PROJECT_NAME} PRIVATE debug ws2_32)



# Compiler warnings & definitions

target_compile_definitions(${PROJECT_NAME} PRIVATE _USE_MATH_DEFINES)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DAK_OPTIMIZED")

if (CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Wno-unknown-pragmas -Wno-gnu-zero-variadic-macro-arguments)
endif()

if (MSVC)
    source_group("src" FILES ${SRC_grunka})
    source_group("foundation" FILES ${bitsquidfoundation_SOURCE_DIR})
    source_group("wwise" FILES ${SRC_AK})
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    set_source_files_properties(${SRC_grunka} PROPERTIES COMPILE_FLAGS "/W4")
    set_source_files_properties(${SRC_AK} PROPERTIES COMPILE_FLAGS "/Zc:wchar_t")
    target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE=1)

    if (LIVE_PP)
        target_compile_definitions(${PROJECT_NAME} PRIVATE LIVE_PP=1 NOMINMAX=1)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zi /hotpatch /Gm-)
        target_link_options(${PROJECT_NAME} PRIVATE /FUNCTIONPADMIN /OPT:NOREF /OPT:NOICF)
    endif()

    if (SUPERLUMINAL)
        target_compile_definitions(${PROJECT_NAME} PRIVATE SUPERLUMINAL=1)
    endif()
endif()
