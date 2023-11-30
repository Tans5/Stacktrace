#include <jni.h>
#include <link.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <sys/mman.h>
#include "waithook.h"
#include "../android_log.h"

static jobject mainAct = nullptr;
static JNIEnv *jniEnv = nullptr;

void *my_malloc(size_t s) {
    if (mainAct != nullptr && jniEnv != nullptr) {
        auto clazz = jniEnv->FindClass("com/tans/stacktrace/MainActivity");
        auto methodId = jniEnv->GetMethodID(clazz, "hookMessage", "(Ljava/lang/String;)V");
        char *chars = static_cast<char *>(malloc(50));
        sprintf(chars, "Alloc %d bytes", s);
        auto jString = jniEnv->NewStringUTF(chars);
        jniEnv->CallVoidMethod(mainAct, methodId, jString);
        LOGD("my_alloc: %d", s);
    }
    return malloc(s);
}

int phdr_callback(struct dl_phdr_info* info, size_t size, void* data) {
    if (strstr(info->dlpi_name, "libwaithook.so")) {
        auto *baseAddr = static_cast<unsigned long *>(data);
        *baseAddr = info->dlpi_addr;
    }
    return 0;
}

void hookMalloc() {
    unsigned long waitHookLibBaseAddr = 0;
    dl_iterate_phdr(phdr_callback, &waitHookLibBaseAddr);
    if (waitHookLibBaseAddr != 0) {
        LOGD("Find libwaithook.so base address: 0x%016lx", waitHookLibBaseAddr);
    } else {
        LOGE("Don't find libwaithook.so base address.");
        return;
    }
    void ** mallocGotAddr = reinterpret_cast<void **>(waitHookLibBaseAddr + 0x1c08);
    auto pageStart = (PAGE_MASK & (unsigned long)mallocGotAddr);
    auto pageEnd = pageStart + PAGE_SIZE;
    //add write permission
    mprotect((void *)pageStart, PAGE_SIZE, PROT_READ | PROT_WRITE);
    *(mallocGotAddr) = (void *)my_malloc;

    //clear instruction cache
    __builtin___clear_cache(static_cast<char *>((void *) pageStart), static_cast<char *>((void *) pageEnd));
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_tans_stacktrace_MainActivity_sayHello(
        JNIEnv* env,
        jobject act /* this */) {
    jniEnv = env;
    mainAct = act;
    auto msg = allocMessage(20);
    sayHello(msg);
    auto jString = env->NewStringUTF(msg->chars);
    freeMessage(msg);
    jniEnv = nullptr;
    mainAct = nullptr;
    return jString;
}


extern "C" JNIEXPORT void JNICALL
Java_com_tans_stacktrace_MainActivity_hook(
        JNIEnv* env,
        jobject /* this */) {
    hookMalloc();
}
