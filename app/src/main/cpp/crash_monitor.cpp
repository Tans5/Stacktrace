//
// Created by pengcheng.tan on 2023/11/21.
//
#include "crash_monitor.h"
#include <csignal>
#include <cstdlib>
#include <iostream>
#include "dump_stack.h"

static bool isRegisterMonitor;

static void sigHandler(int sig, siginfo_t *sig_info, void * uc) {
    LOGE("Receive native crash: %d", sig);
    DumpStackResult stackResult;
    stackResult.stacks = static_cast<char *>(malloc(
            stackResult.maxSingleStackSize * stackResult.maxStackSize));
    dumpStack(&stackResult);
    char *tempStr = static_cast<char *>(malloc(stackResult.maxSingleStackSize));
    for (int i = 0; i < stackResult.size; i ++) {
        char *str = stackResult.stacks + i * stackResult.maxSingleStackSize;
        memcpy(tempStr, str, stackResult.maxSingleStackSize);
        LOGE("%s", tempStr);
    }
    free(tempStr);
    free(stackResult.stacks);
}

void registerCrashMonitor() {
    if (isRegisterMonitor) {
        return;
    }
    isRegisterMonitor = true;
    struct sigaction sigAction{};
    sigfillset(&sigAction.sa_mask);
    sigAction.sa_flags = SA_RESTART | SA_SIGINFO;
    sigAction.sa_sigaction = sigHandler;
    int ret = sigaction(SIGABRT, &sigAction, nullptr);
    if (ret == 0) {
        LOGD("Register signal success.");
    } else {
        LOGE("Register signal fail: %d", ret);
    }
}
