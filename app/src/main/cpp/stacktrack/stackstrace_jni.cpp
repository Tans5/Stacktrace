#include <jni.h>
#include <cstdlib>
#include <sys/system_properties.h>
#include <iostream>
#include "dump_stack.h"
#include "../android_log.h"
#include "crash_monitor.h"

static JavaVM *gJvm;
static JNIEnv *gEnv;
static jclass gClazz;
static jmethodID gJavaCrashHandleMethodId;

void crashHandle(JNIEnv *env, jobject obj, CrashData *crashData) {
    LOGD("Crash handle receive: %s, %s, %lld", crashData->sigName, crashData -> sigSubName, obj);


    jstring jSigName = env->NewStringUTF(crashData->sigName);
    jstring jSigSubName = env->NewStringUTF(crashData->sigSubName);
    auto stackResult = crashData->stackResult;
    StringsOffsetsResult offsetsResult;
    offsetsResult.offsets = static_cast<int *>(malloc(sizeof(int) * stackResult.stackStrSize));
    offsetsResult.maxOffsetsSize = stackResult.stackSize;
    computeStringsOffsets(stackResult.stacks, &offsetsResult);
    jobjectArray jStacks = env->NewObjectArray(offsetsResult.offsetsSize, env->FindClass("java/lang/String"), nullptr);
    for (int i = 0; i < offsetsResult.offsetsSize; i ++) {
        char *targetStr = stackResult.stacks + offsetsResult.offsets[i];
        auto jString = env->NewStringUTF(targetStr);
        env->SetObjectArrayElement(jStacks, i, jString);
        env->DeleteLocalRef(jString);
    }
    free(offsetsResult.offsets);
    env->CallStaticVoidMethod(
            gClazz,
            gJavaCrashHandleMethodId,
            crashData->tid,
            crashData->sig,
            jSigName,
            crashData->sigSub,
            jSigSubName,
            (long long) crashData->time,
            jStacks);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
    JNIEnv *env;
    jvm->GetEnv((void **)&env, JNI_VERSION_1_6);
    gJvm = jvm;
    gEnv = env;
    auto clazz = env->FindClass("com/tans/stacktrace/MainActivity");
    gClazz = static_cast<jclass>(env->NewGlobalRef(clazz));
    gJavaCrashHandleMethodId = env->GetStaticMethodID(clazz, "handleNativeCrash",
                           "(IILjava/lang/String;ILjava/lang/String;J[Ljava/lang/String;)V");

    return JNI_VERSION_1_6;
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
    registerCrashMonitor(gJvm, gEnv, obj, cacheFilePath, cacheFilePathLen, crashHandle);
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
    stackResult.stacks = static_cast<char *>(malloc(stackResult.maxStackStrSize));
    add10DumpStack(20, &stackResult);
    printStackResult(&stackResult);
    StringsOffsetsResult offsetsResult;
    offsetsResult.offsets = static_cast<int *>(malloc(stackResult.stackSize * sizeof(int)));
    offsetsResult.maxOffsetsSize = stackResult.stackSize;
    computeStringsOffsets(stackResult.stacks, &offsetsResult);
    jobjectArray jStacks = env->NewObjectArray(offsetsResult.offsetsSize, env->FindClass("java/lang/String"), nullptr);
    for (int i = 0; i < offsetsResult.offsetsSize; i ++) {
        char *targetStr = stackResult.stacks + offsetsResult.offsets[i];
        auto jString = env->NewStringUTF(targetStr);
        env->SetObjectArrayElement(jStacks, i, jString);
        env->DeleteLocalRef(jString);
    }
    free(offsetsResult.offsets);
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