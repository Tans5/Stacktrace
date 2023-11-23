#include <jni.h>
#include <cstdlib>
#include <sys/system_properties.h>
#include <iostream>
#include "dump_stack.h"
#include "android_log.h"
#include "crash_monitor.h"

static jclass javaCrashHandleClazz = nullptr;
static jmethodID javaCrashHandleMethodId = nullptr;

void crashHandle(JNIEnv *env, jobject obj, CrashData *crashData) {
    LOGD("Crash handle receive: %s, %s, %lld", crashData->sigName, crashData -> sigSubName, obj);


    jstring jSigName = env->NewStringUTF(crashData->sigName);
    jstring jSigSubName = env->NewStringUTF(crashData->sigSubName);
    auto stackResult = crashData->stackResult;
    jobjectArray jStacks = env->NewObjectArray(stackResult.size, env->FindClass("java/lang/String"), nullptr);
    for (int i = 0; i < stackResult.size; i ++) {
        char *tempStackStr = static_cast<char *>(malloc(stackResult.maxSingleStackSize));
        char *targetStr = stackResult.stacks + i * stackResult.maxSingleStackSize;
        memcpy(tempStackStr, targetStr, stackResult.maxSingleStackSize);
        auto jString = env->NewStringUTF(tempStackStr);
        env->SetObjectArrayElement(jStacks, i, jString);
        env->DeleteLocalRef(jString);
        free(tempStackStr);
    }
    env->CallStaticVoidMethod(
            javaCrashHandleClazz,
            javaCrashHandleMethodId ,
            crashData->tid,
            crashData->sig,
            jSigName,
            crashData->sigSub,
            jSigSubName,
            (long long) crashData->time,
            jStacks);
}

extern "C" JNIEXPORT void JNICALL
Java_com_tans_stacktrace_MainActivity_registerCrashMonitor(
        JNIEnv* env,
        jobject obj, /* this */
        jstring cacheFile) {
    jboolean copy = JNI_FALSE;
    env->GetStringLength(cacheFile);
    const char *cacheFilePath = env->GetStringUTFChars(cacheFile, &copy);
    int cacheFilePathLen = env->GetStringLength(cacheFile);
    if (javaCrashHandleMethodId == nullptr) {
        jclass clazz = env->GetObjectClass(obj);
        javaCrashHandleClazz = clazz;
        javaCrashHandleMethodId = env->GetStaticMethodID(clazz, "handleNativeCrash",
                                            "(IILjava/lang/String;ILjava/lang/String;J[Ljava/lang/String;)V");
    }
    registerCrashMonitor(env, obj, cacheFilePath, cacheFilePathLen, crashHandle);
}

int add5DumpStack(int num,  DumpStackResult* dumpResult) {
    dumpStack(dumpResult, 1);
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
    printStackResult(&stackResult);
    jobjectArray jStacks = env->NewObjectArray(stackResult.size, env->FindClass("java/lang/String"), nullptr);
    for (int i = 0; i < stackResult.size; i ++) {
        char *tempStackStr = static_cast<char *>(malloc(stackResult.maxSingleStackSize));
        char *targetStr = stackResult.stacks + i * stackResult.maxSingleStackSize;
        memcpy(tempStackStr, targetStr, stackResult.maxSingleStackSize);
        auto jString = env->NewStringUTF(tempStackStr);
        env->SetObjectArrayElement(jStacks, i, jString);
        env->DeleteLocalRef(jString);
        free(tempStackStr);
    }
    free(stackResult.stacks);
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