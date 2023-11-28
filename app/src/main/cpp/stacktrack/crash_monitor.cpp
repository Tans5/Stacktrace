#include "crash_monitor.h"
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <wait.h>
#include <asm/sigcontext.h>
#include <sys/eventfd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include "dump_stack.h"

static bool isRegisterMonitor;
static bool hasReceiveSig;
static pthread_t crashHandleThread;
static int crashNotifyFd = -1;
static char *cacheFile = nullptr;

static int crashTid = -1;
static int crashSig = -1;
static int crashSigSub = -1;
static long crashTime = -1;

typedef struct CrashHandleThreadArgs {
    JNIEnv *env;
    JavaVM *jvm;
    jobject obj;
    void (* crashHandle)(JNIEnv *, jobject, CrashData*);
} CrashHandleThreadArgs;

typedef struct sigaction sigaction_s;

typedef struct SigActionInfo {
    int sig;
    sigaction_s *oldAct = new sigaction_s;
} SigActionInfo;

static SigActionInfo sigActionInfos[] = {
        {.sig = SIGABRT},
        {.sig = SIGBUS},
        {.sig = SIGFPE},
        {.sig = SIGILL},
        {.sig = SIGSEGV},
        {.sig = SIGTRAP},
        {.sig = SIGSYS},
        {.sig = SIGSTKFLT}
};

static sigaction_s *findOldAct(int sig) {
    int size = sizeof(sigActionInfos) / sizeof(SigActionInfo);
    sigaction_s *result = nullptr;
    for (int i = 0; i < size; i ++) {
        SigActionInfo info = sigActionInfos[i];
        if (info.sig == sig) {
            result = info.oldAct;
            break;
        }
    }
    return result;
}

const char* getSigName(int sig)
{
    switch (sig)
    {
        case SIGABRT:   return "SIGABRT";
        case SIGBUS:    return "SIGBUS";
        case SIGFPE:    return "SIGFPE";
        case SIGILL:    return "SIGILL";
        case SIGSEGV:   return "SIGSEGV";
        case SIGTRAP:   return "SIGTRAP";
        case SIGSYS:    return "SIGSYS";
        case SIGSTKFLT: return "SIGSTKFLT";
        default:        return "?";
    }
}

const char* getSigSubName(int sig, int sigSub)
{
    switch (sig) {
        case SIGBUS:
            switch(sigSub)
            {
                case BUS_ADRALN:    return "BUS_ADRALN";
                case BUS_ADRERR:    return "BUS_ADRERR";
                case BUS_OBJERR:    return "BUS_OBJERR";
                case BUS_MCEERR_AR: return "BUS_MCEERR_AR";
                case BUS_MCEERR_AO: return "BUS_MCEERR_AO";
                default:            break;
            }
            break;
        case SIGFPE:
            switch(sigSub)
            {
                case FPE_INTDIV:   return "FPE_INTDIV";
                case FPE_INTOVF:   return "FPE_INTOVF";
                case FPE_FLTDIV:   return "FPE_FLTDIV";
                case FPE_FLTOVF:   return "FPE_FLTOVF";
                case FPE_FLTUND:   return "FPE_FLTUND";
                case FPE_FLTRES:   return "FPE_FLTRES";
                case FPE_FLTINV:   return "FPE_FLTINV";
                case FPE_FLTSUB:   return "FPE_FLTSUB";
                default:           break;
            }
            break;
        case SIGILL:
            switch(sigSub)
            {
                case ILL_ILLOPC:   return "ILL_ILLOPC";
                case ILL_ILLOPN:   return "ILL_ILLOPN";
                case ILL_ILLADR:   return "ILL_ILLADR";
                case ILL_ILLTRP:   return "ILL_ILLTRP";
                case ILL_PRVOPC:   return "ILL_PRVOPC";
                case ILL_PRVREG:   return "ILL_PRVREG";
                case ILL_COPROC:   return "ILL_COPROC";
                case ILL_BADSTK:   return "ILL_BADSTK";
                default:           break;
            }
            break;
        case SIGSEGV:
            switch(sigSub)
            {
                case SEGV_MAPERR:  return "SEGV_MAPERR";
                case SEGV_ACCERR:  return "SEGV_ACCERR";
                case SEGV_BNDERR:  return "SEGV_BNDERR";
                case SEGV_PKUERR:  return "SEGV_PKUERR";
                default:           break;
            }
            break;
        case SIGTRAP:
            switch(sigSub)
            {
                case TRAP_BRKPT:   return "TRAP_BRKPT";
                case TRAP_TRACE:   return "TRAP_TRACE";
                case TRAP_BRANCH:  return "TRAP_BRANCH";
                case TRAP_HWBKPT:  return "TRAP_HWBKPT";
                default:           break;
            }
            if((sigSub & 0xff) == SIGTRAP)
            {
                switch((sigSub >> 8) & 0xff)
                {
                    case PTRACE_EVENT_FORK:       return "PTRACE_EVENT_FORK";
                    case PTRACE_EVENT_VFORK:      return "PTRACE_EVENT_VFORK";
                    case PTRACE_EVENT_CLONE:      return "PTRACE_EVENT_CLONE";
                    case PTRACE_EVENT_EXEC:       return "PTRACE_EVENT_EXEC";
                    case PTRACE_EVENT_VFORK_DONE: return "PTRACE_EVENT_VFORK_DONE";
                    case PTRACE_EVENT_EXIT:       return "PTRACE_EVENT_EXIT";
                    case PTRACE_EVENT_SECCOMP:    return "PTRACE_EVENT_SECCOMP";
                    case PTRACE_EVENT_STOP:       return "PTRACE_EVENT_STOP";
                    default:                      break;
                }
            }
            break;
        case SIGSYS:
            switch(sigSub)
            {
                case SYS_SECCOMP: return "SYS_SECCOMP";
                default:          break;
            }
            break;
        default:
            break;
    }

    // Then the other codes...
    switch (sigSub) {
        case SI_USER:     return "SI_USER";
        case SI_KERNEL:   return "SI_KERNEL";
        case SI_QUEUE:    return "SI_QUEUE";
        case SI_TIMER:    return "SI_TIMER";
        case SI_MESGQ:    return "SI_MESGQ";
        case SI_ASYNCIO:  return "SI_ASYNCIO";
        case SI_SIGIO:    return "SI_SIGIO";
        case SI_TKILL:    return "SI_TKILL";
        case SI_DETHREAD: return "SI_DETHREAD";
    }

    // Then give up...
    return "?";
}

static void sigHandler(int sig, siginfo_t *sig_info, void *uc) {
    // auto * uctx = static_cast<ucontext *>(uc);
    auto tid = gettid();
    LOGE("Receive native crash: sig=%d, tid=%d", sig, tid);
    auto oldAct = findOldAct(sig);
    if (hasReceiveSig) {
        LOGE("Skip handle signal.");
        if (oldAct != nullptr) {
            oldAct->sa_sigaction(sig, sig_info, uc);
        }
        return;
    }
    hasReceiveSig = true;
    pid_t pid = fork();
    if (pid == 0) {
        // New process
        LOGD("Dump process start");
        DumpStackResult stackResult;
        stackResult.stacks = static_cast<char *>(malloc(stackResult.maxStackStrSize));
        memset(stackResult.stacks, 0, stackResult.maxStackStrSize);
        dumpStack(&stackResult, 1);
        printStackResult(&stackResult);
        int cacheFd = open(cacheFile, O_WRONLY);
        if (cacheFd > 0) {
            int writeCount = 0;
            writeCount += write(cacheFd, &stackResult.stackSize, sizeof(stackResult.stackSize));
            writeCount += write(cacheFd, &stackResult.stackStrSize, sizeof(stackResult.stackStrSize));
            writeCount += write(cacheFd, &stackResult.maxStackSize, sizeof(stackResult.maxStackSize));
            writeCount += write(cacheFd, &stackResult.maxStackStrSize, sizeof(stackResult.maxStackStrSize));
            writeCount += write(cacheFd, stackResult.stacks, stackResult.stackStrSize);
            close(cacheFd);
            LOGD("Write to cache, writeSize=%d", writeCount);
        } else {
            LOGE("Open cache file: %s, fail: %d", cacheFile, cacheFd);
        }
        LOGD("Crash handle thread work finished.");
        free(stackResult.stacks);
        _Exit(0);
    }
    if (pid > 0) {
        // Current process
        LOGD("Waiting dump process: %d", pid);
        int status;
        waitpid(pid, &status, __WALL);
        LOGD("Waiting crash handle thread.");
        timeval tv{};
        gettimeofday(&tv, nullptr);
        long timeInMillis = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
        crashTid = tid;
        crashSig = sig;
        crashSigSub = sig_info->si_code;
        crashTime = timeInMillis;
        long data = 233;
        write(crashNotifyFd, &data, sizeof(long));
        if (crashNotifyFd > 0) {
            close(crashNotifyFd);
            crashNotifyFd = -1;
        }
        pthread_join(crashHandleThread, nullptr);
        LOGD("Dump process finished: %d", status);
        if (oldAct != nullptr) {
            oldAct->sa_sigaction(sig, sig_info, uc);
        }
    }
}

static void* crashHandleRoutine(void* args) {
    auto *args_t = static_cast<CrashHandleThreadArgs *>(args);
    auto *jniEnv = args_t->env;
    LOGD("Crash handle thread started.");
    long data;
    if (crashNotifyFd > 0) {
        JavaVMAttachArgs jvmAttachArgs {
          .version = JNI_VERSION_1_6,
          .name = "CrashHandleThread",
          .group = nullptr
        };
        if (args_t->jvm->AttachCurrentThread(&jniEnv, &jvmAttachArgs) != JNI_OK) {
            LOGE("Attach jvm thread fail.");
            return nullptr;
        }
        while (true) {
            read(crashNotifyFd, &data, sizeof(data));
            LOGD("Crash handle read data: %ld", data);
            if (data == 233L) {
                LOGD("Crash handle thread receive crash, waiting crash handle thread: %ld", crashHandleThread);
                close(crashNotifyFd);
                crashNotifyFd = -1;
                struct stat cacheStat {};
                stat(cacheFile, &cacheStat);
                long long cacheSize = cacheStat.st_size;
                LOGD("Cache file stackSize: %lld", cacheSize);
                int cacheFd = 0;
                if (cacheSize > 0) {
                    cacheFd = open(cacheFile, O_RDONLY);
                }
                if (cacheFd > 0) {
                    int stackSize, stackStrSize, maxStackSize, maxStackStrSize;
                    read(cacheFd, &stackSize, sizeof(int));
                    read(cacheFd, &stackStrSize, sizeof(int));
                    read(cacheFd, &maxStackSize, sizeof(int));
                    read(cacheFd, &maxStackStrSize, sizeof(int));
                    LOGD("StackSize=%d, StackStrSize=%d, MaxStackSize=%d, MasStackStrSize=%d", stackSize, stackStrSize, maxStackSize, maxStackStrSize);
                    if (stackSize > 0 && stackStrSize > 0) {
                        char *stacks = static_cast<char *>(malloc(stackStrSize));
                        int readStackStrSize = read(cacheFd, stacks, stackStrSize);
                        if (readStackStrSize != stackStrSize) {
                            LOGE("Wrong stack str size: %d, needed: %d", readStackStrSize, stackStrSize);
                            break;
                        }
                        CrashData crashData {
                            .tid = crashTid,
                            .sig = crashSig,
                            .sigName = getSigName(crashSig),
                            .sigSub = crashSigSub,
                            .sigSubName = getSigSubName(crashSig, crashSigSub),
                            .time = crashTime,
                            .stackResult = DumpStackResult {
                                .stackSize = stackSize,
                                .stackStrSize = stackStrSize,
                                .maxStackSize = maxStackSize,
                                .maxStackStrSize = maxStackStrSize,
                                .stacks = stacks
                            }
                        };
                        args_t->crashHandle(jniEnv, args_t->obj, &crashData);
                        free(stacks);
                    } else {
                        LOGE("Wrong stackSize.");
                    }
                    close(cacheFd);
                } else {
                    LOGE("Read cache file fail: %d", cacheFd);
                }
                break;
            }
        }
        args_t->jvm->DetachCurrentThread();
    } else {
        LOGE("Crash handle fail: crash notify fd is invalid.");
    }
    free(args);
    return nullptr;
}

void registerCrashMonitor(JavaVM* jvm, JNIEnv *env, jobject obj, const char *cacheFilePath, int cacheFilePathLen, void (* crashHandle)(JNIEnv *, jobject, CrashData*)) {
    if (isRegisterMonitor) {
        return;
    }
    isRegisterMonitor = true;
    if (cacheFile != nullptr) {
        free((void *) cacheFile);
    }
    cacheFile = static_cast<char *>(malloc(cacheFilePathLen + 1));
    memcpy(cacheFile, cacheFilePath, cacheFilePathLen + 1);
    stack_t newStack;
    newStack.ss_flags = 0;
    int crashStackSize = 1024 * 128;
    newStack.ss_size = crashStackSize;
    newStack.ss_sp = malloc(crashStackSize);
    sigaltstack(&newStack, nullptr);
    sigaction_s sigAction{};
    sigfillset(&sigAction.sa_mask);
    sigAction.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
    sigAction.sa_sigaction = sigHandler;
    int sigActionInfoSize = sizeof(sigActionInfos) / sizeof(SigActionInfo);
    int ret = 1;
    for (int i = 0; i < sigActionInfoSize; i ++) {
        SigActionInfo info = sigActionInfos[i];
        if (sigaction(info.sig, &sigAction, info.oldAct) == 0) {
            ret = 0;
        }
    }
    if (ret == 0) {
        LOGD("Register signal success.");
        crashNotifyFd = eventfd(0, EFD_CLOEXEC);
        if (crashNotifyFd > 0) {
            LOGD("Create notify fd success: %d", crashNotifyFd);
            auto *args = new CrashHandleThreadArgs;
            args->env = env;
            args->jvm = jvm;
            args->obj = obj;
            args->crashHandle = crashHandle;
            ret = pthread_create(&crashHandleThread, nullptr, crashHandleRoutine, args);
            if (ret == 0) {
                LOGD("Create crash handle thread success.");
            } else {
                LOGE("Create crash handle thread fail: %d", ret);
            }
        } else {
            LOGE("Create notify fd fail: %d", crashNotifyFd);
        }
    } else {
        LOGE("Register signal fail: %d", ret);
    }
}
