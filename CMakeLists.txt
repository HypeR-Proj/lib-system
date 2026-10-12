# SPDX-FileCopyrightText: 2026 roolrz
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.25)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
if(NOT DEFINED CMAKE_AR)
    find_program(CMAKE_AR NAMES llvm-ar REQUIRED)
endif()
if(NOT DEFINED CMAKE_RANLIB)
    find_program(CMAKE_RANLIB NAMES llvm-ranlib REQUIRED)
endif()
project(HypeRLib VERSION 0.0.0 LANGUAGES C ASM)

if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "HypeR Lib requires Clang")
endif()

set(HYPER_ARCH "aarch64" CACHE STRING "HypeR target architecture")
set(HYPER_ABI_INCLUDE_DIR "" CACHE PATH "Directory containing hyper/native.h")

if(NOT HYPER_ARCH MATCHES "^(aarch64|riscv64)$")
    message(FATAL_ERROR "Native userspace is requires a supported Native architecture")
endif()
if(NOT EXISTS "${HYPER_ABI_INCLUDE_DIR}/hyper/native.h")
    message(FATAL_ERROR "HYPER_ABI_INCLUDE_DIR must contain hyper/native.h")
endif()

set(HYPER_RUNTIME_SOURCES
    src/arch/${HYPER_ARCH}/syscall.S
    src/system.c
    src/runtime.c
    src/bootstrap.c
    src/launch.c
    src/arch/${HYPER_ARCH}/stack.S
    src/capabilities.c
    src/directory.c
    src/thread.c
    src/thread_spawn.c
    src/stack.c
    src/stdio.c
    src/heap.c
    src/stdlib.c
    src/dlfcn.c
    src/memory.c
    src/startup.c
    src/string.c
    src/syscall.c
)

add_library(hyper STATIC ${HYPER_RUNTIME_SOURCES})
add_library(hyper_shared_objects OBJECT ${HYPER_RUNTIME_SOURCES})

add_library(hyper_crt1 OBJECT src/arch/${HYPER_ARCH}/crt1.S)
add_library(hyper_crt OBJECT src/crt.c)
add_library(hyper_crt_std OBJECT src/crt-std.c)
add_library(hyper_std STATIC src/std.c src/std-fs.c src/std-process.c)
set_target_properties(hyper_std PROPERTIES OUTPUT_NAME hyper-std)
foreach(runtime_target hyper_crt_std hyper_std)
    target_compile_features(${runtime_target} PRIVATE c_std_17)
    target_compile_options(${runtime_target} PRIVATE -ffreestanding -fno-builtin -fPIC -Wall -Wextra -Werror)
    target_include_directories(${runtime_target} PRIVATE include ${HYPER_ABI_INCLUDE_DIR})
endforeach()
target_compile_features(hyper_crt PRIVATE c_std_17)
target_compile_options(hyper_crt PRIVATE -ffreestanding -fno-builtin -fPIE -Wall -Wextra -Werror)
target_include_directories(hyper_crt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${HYPER_ABI_INCLUDE_DIR}
)

target_compile_features(hyper PRIVATE c_std_17)
target_compile_options(hyper PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-ffreestanding;-fno-builtin;-fPIC;-fvisibility=hidden;-Wall;-Wextra;-Werror>
)
target_compile_features(hyper_shared_objects PRIVATE c_std_17)
target_compile_options(hyper_shared_objects PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-ffreestanding;-fno-builtin;-fPIC;-Wall;-Wextra;-Werror>
    $<$<COMPILE_LANGUAGE:ASM>:-fPIC>
)
target_include_directories(hyper
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${HYPER_ABI_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)
target_include_directories(hyper_shared_objects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${HYPER_ABI_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)
set_target_properties(hyper PROPERTIES OUTPUT_NAME hyper)

if(NOT HYPER_LD)
    message(FATAL_ERROR "HYPER_LD must name ld.lld")
endif()
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libhyper.so
    COMMAND ${HYPER_LD}
        -shared $<TARGET_OBJECTS:hyper_shared_objects>
        --no-undefined --fatal-warnings -Bsymbolic -z text -z relro -z now
        --build-id=none --gc-sections -z noexecstack -z max-page-size=4096
        --hash-style=sysv -soname libhyper.so
        -o ${CMAKE_CURRENT_BINARY_DIR}/libhyper.so
    DEPENDS hyper_shared_objects
    COMMAND_EXPAND_LISTS
    VERBATIM
)
add_custom_target(hyper_shared ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libhyper.so)

install(TARGETS hyper ARCHIVE DESTINATION lib)
install(TARGETS hyper_std ARCHIVE DESTINATION lib)
install(FILES $<TARGET_OBJECTS:hyper_crt_std> DESTINATION lib RENAME crt-std.o)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libhyper.so DESTINATION lib)
install(FILES $<TARGET_OBJECTS:hyper_crt1> DESTINATION lib RENAME crt1.o)
install(FILES $<TARGET_OBJECTS:hyper_crt> DESTINATION lib RENAME crt.o)
install(DIRECTORY include/ DESTINATION include)
