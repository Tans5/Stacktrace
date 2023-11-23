#ifndef STACKTRACE_CRASH_MONITOR_H
#define STACKTRACE_CRASH_MONITOR_H

#include <jni.h>
#include "dump_stack.h"
#include "android_log.h"

typedef struct CrashData {
    int tid = 0;
    int sig = 0;
    const char *sigName;
    int sigSub = 0;
    const char *sigSubName;
    long time = 0;
    DumpStackResult stackResult;
} CrashData;

void registerCrashMonitor(JNIEnv* env, jobject obj, const char * cacheFilePath, int cacheFilePathLen, void (* crashHandle)(JNIEnv *, jobject, CrashData*));

#endif //STACKTRACE_CRASH_MONITOR_H
