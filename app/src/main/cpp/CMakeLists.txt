# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

cmake_minimum_required(VERSION 3.22.1)

project("stacktrace")

add_library(${CMAKE_PROJECT_NAME} SHARED
        stacktrack/stackstrace_jni.cpp
        stacktrack/dump_stack.cpp
        stacktrack/dump_stack.h
        android_log.h
        stacktrack/crash_monitor.h
        stacktrack/crash_monitor.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        android
        log)

target_compile_options(${CMAKE_PROJECT_NAME} PUBLIC -fsanitize=address -fno-omit-frame-pointer)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS -fsanitize=address)

add_library(memalloctest SHARED
        memalloctest/mem_alloc_test_jni.cpp)

target_link_libraries(memalloctest
        android
        log)

target_compile_options(memalloctest PUBLIC -fsanitize=address -fno-omit-frame-pointer)
set_target_properties(memalloctest PROPERTIES LINK_FLAGS -fsanitize=address)