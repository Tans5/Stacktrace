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

add_library(memalloctest SHARED
        memalloctest/mem_alloc_test_jni.cpp)

target_link_libraries(memalloctest
        android
        log)