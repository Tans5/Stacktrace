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
#include "dump_stack.h"

static bool isRegisterMonitor;
static bool hasReceiveSig;
static struct sigaction oldAct;
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
    void (* crashHandle)(JNIEnv *, CrashData*);
} CrashHandleThreadArgs;

static void sigHandler(int sig, siginfo_t *sig_info, void *uc) {
    auto * uctx = static_cast<ucontext *>(uc);
    auto tid = gettid();
    LOGE("Receive native crash: sig=%d, tid=%d", sig, tid);
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
        dumpStack(&stackResult, 1);
        printStackResult(&stackResult);
        int cacheFd = open(cacheFile, O_WRONLY);
        if (cacheFd > 0) {
            int writeCount = 0;
            writeCount += write(cacheFd, &stackResult.size, sizeof(stackResult.size));
            writeCount += write(cacheFd, &stackResult.maxSingleStackSize, sizeof(stackResult.maxSingleStackSize));
            writeCount += write(cacheFd, &stackResult.maxStackSize, sizeof(stackResult.maxStackSize));
            writeCount += write(cacheFd, stackResult.stacks, stackResult.maxSingleStackSize * stackResult.maxStackSize);
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
        oldAct.sa_sigaction(sig, sig_info, uc);
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
                LOGD("Cache file size: %lld", cacheSize);
                int cacheFd = 0;
                if (cacheSize > 0) {
                    cacheFd = open(cacheFile, O_RDONLY);
                }
                if (cacheFd > 0) {
                    int crashStackSize, maxSingleStackSize, maxStackSize;
                    read(cacheFd, &crashStackSize, sizeof(int));
                    read(cacheFd, &maxSingleStackSize, sizeof(int));
                    read(cacheFd, &maxStackSize, sizeof(int));
                    LOGD("CrashStackSize=%d, MaxSingleStackSize=%d, MaxStackSize=%d", crashStackSize, maxSingleStackSize, maxStackSize);
                    if (crashStackSize > 0 && maxSingleStackSize > 0 && maxStackSize > 0) {
                        char *stacks = static_cast<char *>(malloc(crashStackSize * maxSingleStackSize));
                        read(cacheFd, stacks, crashStackSize * maxSingleStackSize);
                        LOGD("Read stacks: %s", stacks);
                        CrashData crashData {
                            .tid = crashTid,
                            .sig = crashSig,
                            .sigSub = crashSigSub,
                            .time = crashTime,
                            .stackResult = DumpStackResult {
                                .size = crashStackSize,
                                .maxSingleStackSize = maxSingleStackSize,
                                .maxStackSize = maxStackSize,
                                .stacks=stacks
                            }
                        };
                        args_t->crashHandle(jniEnv, &crashData);
                        free(stacks);
                    } else {
                        LOGE("Wrong size.");
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

void registerCrashMonitor(JNIEnv *env, const char *cacheFilePath, int cacheFilePathLen, void (* crashHandle)(JNIEnv *, CrashData*)) {
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
    struct sigaction sigAction{};
    sigfillset(&sigAction.sa_mask);
    sigAction.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
    sigAction.sa_sigaction = sigHandler;
    int ret = sigaction(SIGABRT, &sigAction, &oldAct);
    if (ret == 0) {
        LOGD("Register signal success.");
        crashNotifyFd = eventfd(0, EFD_CLOEXEC);
        if (crashNotifyFd > 0) {
            LOGD("Create notify fd success: %d", crashNotifyFd);
            JavaVM *jvm;
            env->GetJavaVM(&jvm);
            auto *args = new CrashHandleThreadArgs;
            args->env = env;
            args->jvm = jvm;
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
