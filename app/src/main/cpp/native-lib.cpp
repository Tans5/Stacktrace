#include <jni.h>
#include <cstdlib>
#include <sys/system_properties.h>
#include <iostream>
#include "dump_stack.h"
#include "android_log.h"
#include "crash_monitor.h"


extern "C" JNIEXPORT void JNICALL
Java_com_tans_stacktrace_MainActivity_registerCrashMonitor(
        JNIEnv* env,
        jobject /* this */) {
    registerCrashMonitor();
}

int add5DumpStack(int num,  DumpStackResult* dumpResult) {
    dumpStack(dumpResult);
    return num + 5;
}

int add10DumpStack(int num, DumpStackResult* dumpResult) {
    int result1 = add5DumpStack(num, dumpResult);
    return result1;
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_tans_stacktrace_MainActivity_dumpTestThreadStack(
        JNIEnv* env,
        jobject /* this */) {
    DumpStackResult stackResult;
    stackResult.stacks = static_cast<char *>(malloc(stackResult.maxStackSize * stackResult.maxSingleStackSize));
    add10DumpStack(20, &stackResult);
    jobjectArray jStacks = env->NewObjectArray(stackResult.size, env->FindClass("java/lang/String"), nullptr);
    char *tempStackStr = static_cast<char *>(malloc(stackResult.maxStackSize));
    for (int i = 0; i < stackResult.size; i ++) {
        char *targetStr = stackResult.stacks + i * stackResult.maxSingleStackSize;
        memcpy(tempStackStr, targetStr, stackResult.maxSingleStackSize);
        auto jString = env->NewStringUTF(tempStackStr);
        env->SetObjectArrayElement(jStacks, i, jString);
    }
    free(stackResult.stacks);
    free(tempStackStr);
    return jStacks;
}

int add5DumpCrash(int num) {
    abort();
    return num + 5;
}

int add10DumpCrash(int num) {
    int result1 = add5DumpCrash(num);
    return result1;
}

extern "C" JNIEXPORT void JNICALL
Java_com_tans_stacktrace_MainActivity_testCrash(
        JNIEnv* env,
        jobject /* this */) {
    add10DumpCrash(20);
}