//
// Created by pengcheng.tan on 2023/11/27.
//
#include <jni.h>
#include <cstdlib>

static void **testMemArray = nullptr;
static int memArraySize = 0;

extern "C" JNIEXPORT void JNICALL
Java_com_tans_stacktrace_MainActivity_testAllocMem(
        JNIEnv* env,
        jobject /* this */,
        jlong size) {
    if (size <= 0) return;
    testMemArray = static_cast<void **>(realloc(testMemArray, (++memArraySize) * sizeof(void *)));
    auto data = malloc(size);
    testMemArray[memArraySize - 1] = data;
    char *cData = static_cast<char *>(data);
    for (int i = 0; i < size; i ++) {
        cData[i] = 'a';
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_tans_stacktrace_MainActivity_freeAllTestMem(
        JNIEnv* env,
        jobject /* this */) {
    if (memArraySize <= 0) return;
    for (int i = 0; i < memArraySize; i ++) {
        auto data = testMemArray[i];
        testMemArray[i] = nullptr;
        free(data);
    }
    free(testMemArray);
    testMemArray = nullptr;
    memArraySize = 0;
}