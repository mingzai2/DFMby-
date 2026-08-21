#pragma once

#include <jni.h>
#include <string>
#include <vector>
#include <mutex>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "sha256_util.hpp"

#define LOG_TAG "DRMCleaner"

#if defined(__aarch64__)
#define ABI "arm64-v8a"
#elif defined(__arm__)
#define ABI "armeabi-v7a"
#elif defined(__x86_64__)
#define ABI "x86_64"
#elif defined(__i386__)
#define ABI "x86"
#else
#define ABI "unknown"
#endif

namespace ming {

class Logger {
public:
    static void d(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        __android_log_vprint(ANDROID_LOG_DEBUG, LOG_TAG, fmt, args);
        va_end(args);
    }
    static void i(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        __android_log_vprint(ANDROID_LOG_INFO, LOG_TAG, fmt, args);
        va_end(args);
    }
    static void w(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        __android_log_vprint(ANDROID_LOG_WARN, LOG_TAG, fmt, args);
        va_end(args);
    }
    static void e(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        __android_log_vprint(ANDROID_LOG_ERROR, LOG_TAG, fmt, args);
        va_end(args);
    }
};

}

