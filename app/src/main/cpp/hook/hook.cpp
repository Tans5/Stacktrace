#include <jni.h>
#include "waithook.h"
#include "../android_log.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_tans_stacktrace_MainActivity_sayHello(
        JNIEnv* env,
        jobject /* this */) {
    auto msg = allocMessage(20);
    sayHello(msg);
    auto jString = env->NewStringUTF(msg->chars);
    freeMessage(msg);
    return jString;
}
