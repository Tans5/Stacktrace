#ifndef STACKTRACE_CRASH_MONITOR_H
#define STACKTRACE_CRASH_MONITOR_H

#include <jni.h>
#include "dump_stack.h"
#include "android_log.h"

typedef struct CrashData {
    int tid = 0;
    int sig = 0;
    int sigSub = 0;
    long time = 0;
    DumpStackResult stackResult;
} CrashData;

void registerCrashMonitor(JNIEnv* env, const char * cacheFilePath, int cacheFilePathLen, void (* crashHandle)(JNIEnv *, CrashData*));

#endif //STACKTRACE_CRASH_MONITOR_H
