#include "crash_monitor.h"
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <wait.h>
#include <asm/sigcontext.h>
#include "dump_stack.h"

static bool isRegisterMonitor;
static bool hasReceiveSig;
static struct sigaction oldAct;
static void sigHandler(int sig, siginfo_t *sig_info, void *uc) {
    auto * uctx = static_cast<ucontext *>(uc);
    LOGE("Receive native crash: %d", sig);
    if (hasReceiveSig) {
        LOGE("Skip handle signal.");
        oldAct.sa_sigaction(sig, sig_info, uc);
        return;
    }
    hasReceiveSig = true;
    pid_t pid = fork();
    if (pid == 0) {
        // New process
        LOGD("Dump process start");
        DumpStackResult stackResult;
        stackResult.stacks = static_cast<char *>(malloc(
                stackResult.maxSingleStackSize * stackResult.maxStackSize));
        dumpStack(&stackResult, 3);
        printStackResult(&stackResult);
        free(stackResult.stacks);
        _Exit(0);
    }
    if (pid > 0) {
        // Current process
        LOGD("Waiting dump process: %d", pid);
        int status;
        waitpid(pid, &status, __WALL);
        LOGD("Dump process finished: %d", status);
        oldAct.sa_sigaction(sig, sig_info, uc);
    }
}

void registerCrashMonitor() {
    if (isRegisterMonitor) {
        return;
    }
    isRegisterMonitor = true;
    stack_t newStack;
    newStack.ss_flags = 0;
    int crashStackSize = 1024 * 128;
    newStack.ss_size = crashStackSize;
    newStack.ss_sp = malloc(crashStackSize);
    sigaltstack(&newStack, nullptr);
    struct sigaction sigAction{};
    sigfillset(&sigAction.sa_mask);
    sigAction.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
    sigAction.sa_sigaction = sigHandler;
    int ret = sigaction(SIGABRT, &sigAction, &oldAct);
    if (ret == 0) {
        LOGD("Register signal success.");
    } else {
        LOGE("Register signal fail: %d", ret);
    }
}
