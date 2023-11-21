//
// Created by pengcheng.tan on 2023/11/20.
//
#ifndef STACKTRACE_ANDROID_LOG_H
#define STACKTRACE_ANDROID_LOG_H

#include "android/log.h"
#define LOG_TAG "Stacktrace"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#endif //STACKTRACE_ANDROID_LOG_H
