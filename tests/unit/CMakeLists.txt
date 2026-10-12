# SPDX-FileCopyrightText: 2026 roolrz
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.25)
project(HypeRLibUnitTests LANGUAGES C)

if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "HypeR Lib unit tests require Clang")
endif()

enable_testing()

add_executable(hyper-memory-test
    ../../src/memory.c
    ../../src/string.c
    memory.c
)
target_compile_features(hyper-memory-test PRIVATE c_std_17)
target_compile_options(hyper-memory-test PRIVATE
    -ffreestanding
    -fno-builtin
    -Wall
    -Wextra
    -Werror
)
target_include_directories(hyper-memory-test PRIVATE ../../include)

add_test(NAME memory-primitives COMMAND hyper-memory-test)

add_executable(hyper-startup-test
    ../../src/startup.c
    startup.c
)
target_compile_features(hyper-startup-test PRIVATE c_std_17)
target_compile_options(hyper-startup-test PRIVATE
    -Wall
    -Wextra
    -Werror
)
target_include_directories(hyper-startup-test PRIVATE
    ../../include
    ${HYPER_ABI_INCLUDE_DIR}
)
if(NOT EXISTS "${HYPER_ABI_INCLUDE_DIR}/hyper/native.h")
    message(FATAL_ERROR "HYPER_ABI_INCLUDE_DIR must contain hyper/native.h")
endif()

add_test(NAME startup-stack COMMAND hyper-startup-test)

add_executable(hyper-syscall-test
    ../../src/syscall.c
    syscall.c
    syscall-capture.c
)
target_compile_features(hyper-syscall-test PRIVATE c_std_17)
target_compile_options(hyper-syscall-test PRIVATE
    -UNDEBUG
    -Wall
    -Wextra
    -Werror
)
target_include_directories(hyper-syscall-test PRIVATE
    ../../include
    ${HYPER_ABI_INCLUDE_DIR}
)

add_test(NAME syscall-veneer COMMAND hyper-syscall-test)

# Compile the real no_std SDK with the host compiler; no installed SDK/sysroot
# or target executable is needed to verify the Rust/C transport correspondence.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(HYPER_HOST_RUSTC rustc REQUIRED)
add_test(NAME syscall-rust-transport
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check-syscall-rust.py
        --cc ${CMAKE_C_COMPILER} --rustc ${HYPER_HOST_RUSTC})

# Rename only the C allocation facade so the fixture never intercepts host libc.
find_package(Threads REQUIRED)
add_executable(hyper-heap-test mutex-host.c ../../src/system.c ../../src/heap.c ../../src/stdlib.c ../../src/startup.c heap.c)
target_compile_features(hyper-heap-test PRIVATE c_std_17)
target_compile_options(hyper-heap-test PRIVATE -fno-builtin -Wall -Wextra -Werror
    -include ${CMAKE_CURRENT_SOURCE_DIR}/heap-host.h)
target_compile_definitions(hyper-heap-test PRIVATE
    HYPER_HEAP_BASE=hyper_heap_test_base
    malloc=hyper_test_malloc calloc=hyper_test_calloc realloc=hyper_test_realloc
    free=hyper_test_free aligned_alloc=hyper_test_aligned_alloc
)
target_include_directories(hyper-heap-test PRIVATE ../../include ${HYPER_ABI_INCLUDE_DIR})
target_link_libraries(hyper-heap-test PRIVATE Threads::Threads)
add_test(NAME process-heap COMMAND hyper-heap-test)

add_executable(hyper-thread-test ../../src/thread.c thread.c atomic-wait-host.c)
target_compile_features(hyper-thread-test PRIVATE c_std_17)
target_compile_definitions(hyper-thread-test PRIVATE HYPER_THREAD_HOST_TEST _POSIX_C_SOURCE=200809L)
target_compile_options(hyper-thread-test PRIVATE -Wall -Wextra -Werror)
target_include_directories(hyper-thread-test PRIVATE ../../include ${HYPER_ABI_INCLUDE_DIR})
target_link_libraries(hyper-thread-test PRIVATE Threads::Threads)
add_test(NAME thread-runtime COMMAND hyper-thread-test)

add_executable(hyper-stdio-test mutex-host.c ../../src/stdio.c ../../src/startup.c stdio.c)
target_compile_features(hyper-stdio-test PRIVATE c_std_17)
target_compile_options(hyper-stdio-test PRIVATE -Wall -Wextra -Werror)
target_include_directories(hyper-stdio-test PRIVATE ../../include ${HYPER_ABI_INCLUDE_DIR})
add_test(NAME standard-streams COMMAND hyper-stdio-test)

# Real host page protection verifies unmapped stack guards before/after growth.
add_executable(hyper-stack-test mutex-host.c ../../src/system.c ../../src/stack.c ../../src/startup.c stack.c)
target_compile_features(hyper-stack-test PRIVATE c_std_17)
target_compile_options(hyper-stack-test PRIVATE -Wall -Wextra -Werror -UNDEBUG
    -include ${CMAKE_CURRENT_SOURCE_DIR}/stack-host.h)
target_include_directories(hyper-stack-test PRIVATE ../../include ${HYPER_ABI_INCLUDE_DIR})
target_link_libraries(hyper-stack-test PRIVATE Threads::Threads)
add_test(NAME userspace-stacks COMMAND hyper-stack-test)

add_executable(hyper-runtime-startup-test ../../src/bootstrap.c ../../src/runtime.c ../../src/startup.c runtime-startup.c)
target_compile_features(hyper-runtime-startup-test PRIVATE c_std_17)
target_compile_options(hyper-runtime-startup-test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(hyper-runtime-startup-test PRIVATE ../../include ${HYPER_ABI_INCLUDE_DIR})
add_test(NAME runtime-startup-ownership COMMAND hyper-runtime-startup-test)

add_executable(hyper-launch-test ../../src/launch.c launch.c)
target_compile_features(hyper-launch-test PRIVATE c_std_17)
target_compile_options(hyper-launch-test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(hyper-launch-test PRIVATE ../../include ${HYPER_ABI_INCLUDE_DIR})
add_test(NAME launch-result-ownership COMMAND hyper-launch-test)

add_executable(hyper-thread-spawn-test thread-spawn.c)
target_compile_features(hyper-thread-spawn-test PRIVATE c_std_17)
target_compile_options(hyper-thread-spawn-test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(hyper-thread-spawn-test PRIVATE ../../include ${HYPER_ABI_INCLUDE_DIR})
add_test(NAME thread-termination-ownership COMMAND hyper-thread-spawn-test)

add_executable(hyper-system-test ../../src/system.c system.c)
target_compile_features(hyper-system-test PRIVATE c_std_17)
target_compile_options(hyper-system-test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(hyper-system-test PRIVATE ../../include ${HYPER_ABI_INCLUDE_DIR})
foreach(page_size 4096 16384 65536)
    add_test(NAME system-config-${page_size} COMMAND hyper-system-test ${page_size})
endforeach()

add_executable(hyper-mutex-test mutex.c mutex-host.c)
target_compile_features(hyper-mutex-test PRIVATE c_std_17)
target_compile_options(hyper-mutex-test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(hyper-mutex-test PRIVATE ../../include ${HYPER_ABI_INCLUDE_DIR})
target_link_libraries(hyper-mutex-test PRIVATE Threads::Threads)
target_link_libraries(hyper-stdio-test PRIVATE Threads::Threads)
add_test(NAME runtime-mutex COMMAND hyper-mutex-test)
set_tests_properties(runtime-mutex process-heap userspace-stacks PROPERTIES TIMEOUT 30)
