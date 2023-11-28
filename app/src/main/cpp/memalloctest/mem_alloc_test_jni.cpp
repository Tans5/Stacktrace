#include <jni.h>
#include <cstdlib>
#include <cstring>

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
    memset(data, 0, size);
    testMemArray[memArraySize - 1] = data;
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

extern "C" JNIEXPORT void JNICALL
Java_com_tans_stacktrace_MainActivity_testVisitOutBoundArray(
        JNIEnv* env,
        jobject /* this */) {
    int size = sizeof(int) * 5;
    int *array = static_cast<int *>(malloc(size));
    memset(array, 1, size);
    int a = array[5];
    free(array);
}