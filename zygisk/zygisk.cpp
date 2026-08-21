#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <mutex>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <cstdint>

#include "zygisk.hpp"
#include "config_store.hpp"
#include "sha256_util.hpp"

#define TAG "DRMCleaner"

using namespace zygisk;

namespace ming {

ConfigStore g_config;

std::string getCurrentPackageName(JNIEnv* env);

class MingModule : public Module {
public:
    void onModuleLoaded() override {
        Logger::i("=== Ming Module Loaded (by 铭) ===");
        Logger::i("ABI: %s | PID: %d", ABI, getpid());
    }

    void preAppSpecialize(AppSpecializeArgs* args) override {
        JNIEnv* env = args->env;
        if (!env) return;

        const char* pkg = args->nice_name ? args->nice_name : "unknown";
        Logger::d("preAppSpecialize: %s (uid=%d)", pkg, args->uid);

        g_config.init(std::string("/data/adb/modules/mingdfm"));

        setupJniHooks(env, pkg);
    }

    void postAppSpecialize(AppSpecializeArgs* args) override {
        if (!args->nice_name) return;
        Logger::d("postAppSpecialize: %s - hooks active", args->nice_name);
    }

    void preServerSpecialize(ServerSpecializeArgs* args) override {
        Logger::i("preServerSpecialize: uid=%d", args->uid);
        g_config.init(std::string("/data/adb/modules/mingdfm"));
    }

    void postServerSpecialize(ServerSpecializeArgs* args) override {
        Logger::i("postServerSpecialize completed");
    }

private:
    void setupJniHooks(JNIEnv* env, const char* packageName) {
        std::string pkg(packageName ? packageName : "unknown");

        hookMediaDrm(env, pkg);
        hookBuildFields(env, pkg);
        hookSettingsSecure(env, pkg);
        hookTelephonyManager(env, pkg);
    }

    void hookMediaDrm(JNIEnv* env, const std::string& pkg) {
        jclass mediaDrmClass = env->FindClass("android/media/MediaDrm");
        if (!mediaDrmClass) {
            env->ExceptionClear();
            return;
        }

        JNINativeMethod methods[] = {
            {(char*)"getPropertyByteArray", (char*)"(Ljava/lang/String;)[B",
             (void*)native_getPropertyByteArray},
            {(char*)"getUniqueId", (char*)"()[B",
             (void*)native_getUniqueId}
        };

        if (env->RegisterNatives(mediaDrmClass, methods, 2) == 0) {
            Logger::d("[%s] MediaDrm hooked", pkg.c_str());
        } else {
            env->ExceptionClear();
        }

        env->DeleteLocalRef(mediaDrmClass);
    }

    void hookBuildFields(JNIEnv* env, const std::string& pkg) {
        jclass buildClass = env->FindClass("android/os/Build");
        if (!buildClass) {
            env->ExceptionClear();
            return;
        }

        JNINativeMethod serialMethod[] = {
            {(char*)"getSerial", (char*)"()Ljava/lang/String;",
             (void*)native_getSerial}
        };

        if (env->RegisterNatives(buildClass, serialMethod, 1) == 0) {
            Logger::d("[%s] Build::getSerial hooked", pkg.c_str());
        } else {
            env->ExceptionClear();
        }

        env->DeleteLocalRef(buildClass);
    }

    void hookSettingsSecure(JNIEnv* env, const std::string& pkg) {
        jclass settingsClass = env->FindClass("android/provider/Settings$Secure");
        if (!settingsClass) {
            env->ExceptionClear();
            return;
        }

        JNINativeMethod methods[] = {
            {(char*)"getString", (char*)"(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;",
             (void*)native_settingsGetString}
        };

        if (env->RegisterNatives(settingsClass, methods, 1) == 0) {
            Logger::d("[%s] Settings.Secure::getString hooked", pkg.c_str());
        } else {
            env->ExceptionClear();
        }

        env->DeleteLocalRef(settingsClass);
    }

    void hookTelephonyManager(JNIEnv* env, const std::string& pkg) {
        jclass tmClass = env->FindClass("android/telephony/TelephonyManager");
        if (!tmClass) {
            env->ExceptionClear();
            return;
        }

        JNINativeMethod methods[] = {
            {(char*)"getDeviceId", (char*)"()Ljava/lang/String;", (void*)native_getDeviceId},
            {(char*)"getImei", (char*)"()Ljava/lang/String;", (void*)native_getImei},
            {(char*)"getSubscriberId", (char*)"()Ljava/lang/String;", (void*)native_getSubscriberId}
        };

        if (env->RegisterNatives(tmClass, methods, 3) == 0) {
            Logger::d("[%s] TelephonyManager hooked (3 methods)", pkg.c_str());
        } else {
            env->ExceptionClear();
        }

        env->DeleteLocalRef(tmClass);
    }
};

REGISTER_ZYGISK_MODULE(MingModule)

jbyteArray createByteArrayFromHex(JNIEnv* env, const std::string& hexStr) {
    size_t len = hexStr.size() / 2;
    if (len == 0) return nullptr;

    jbyteArray arr = env->NewByteArray((jsize)len);
    if (!arr) return nullptr;

    std::vector<uint8_t> bytes(len);
    for (size_t i = 0; i < len; i++) {
        char buf[3] = {hexStr[i*2], hexStr[i*2+1], '\0'};
        bytes[i] = (uint8_t)strtol(buf, nullptr, 16);
    }
    env->SetByteArrayRegion(arr, 0, (jsize)len, (const jbyte*)bytes.data());
    return arr;
}

extern "C" {

JNIEXPORT jbyteArray JNICALL
native_getPropertyByteArray(JNIEnv* env, jobject thiz, jstring propertyName) {
    if (!propertyName) {
        return nullptr;
    }

    const char* prop = env->GetStringUTFChars(propertyName, nullptr);
    if (!prop) return nullptr;
    std::string propStr(prop);
    env->ReleaseStringUTFChars(propertyName, prop);

    std::string pkg = getCurrentPackageName(env);

    if (propStr == "deviceUniqueId" || propStr == "deviceUniqueID") {
        std::string spoofed = ming::g_config.getSpoofedDrmId(pkg);
        Logger::i("[%s] getPropertyByteArray(%s) -> %zuB",
                  pkg.c_str(), propStr.c_str(), spoofed.size() / 2);
        return createByteArrayFromHex(env, spoofed);
    }

    if (propStr == "systemId" || propStr == "systemID") {
        std::string spoofed = ming::g_config.getSpoofedDrmId(pkg);
        Logger::i("[%s] getPropertyByteArray(%s) -> spoofed", pkg.c_str(), propStr.c_str());
        return createByteArrayFromHex(env, spoofed);
    }

    Logger::d("[%s] getPropertyByteArray(%s) -> passthrough", pkg.c_str(), propStr.c_str());
    return nullptr;
}

JNIEXPORT jbyteArray JNICALL
native_getUniqueId(JNIEnv* env, jobject thiz) {
    std::string pkg = getCurrentPackageName(env);
    std::string spoofed = ming::g_config.getSpoofedDrmId(pkg);

    Logger::i("[%s] getUniqueId() -> spoofed", pkg.c_str());
    return createByteArrayFromHex(env, spoofed);
}

JNIEXPORT jstring JNICALL
native_getSerial(JNIEnv* env, jclass clazz) {
    std::string pkg = getCurrentPackageName(env);
    std::string spoofed = ming::g_config.getSpoofedDeviceId(pkg);

    Logger::d("[%s] getSerial() -> %s", pkg.c_str(), spoofed.c_str());
    return env->NewStringUTF(spoofed.c_str());
}

JNIEXPORT jstring JNICALL
native_settingsGetString(JNIEnv* env, jclass clazz, jobject resolver, jstring name) {
    if (!name) return nullptr;

    const char* nameStr = env->GetStringUTFChars(name, nullptr);
    if (!nameStr) return nullptr;
    std::string key(nameStr);
    env->ReleaseStringUTFChars(name, nameStr);

    std::string pkg = getCurrentPackageName(env);

    if (key == "android_id" || key == "ANDROID_ID") {
        std::string spoofed = ming::g_config.getSpoofedAndroidId(pkg);
        Logger::d("[%s] Settings.Secure.ANDROID_ID -> %s", pkg.c_str(), spoofed.c_str());
        return env->NewStringUTF(spoofed.c_str());
    }

    if (key == "device_provisioned") {
        return env->NewStringUTF("1");
    }

    return nullptr;
}

JNIEXPORT jstring JNICALL
native_getDeviceId(JNIEnv* env, jobject thiz) {
    std::string pkg = getCurrentPackageName(env);
    std::string spoofed = ming::g_config.getSpoofedDeviceId(pkg);
    Logger::d("[%s] getDeviceId() -> spoofed", pkg.c_str());
    return env->NewStringUTF(spoofed.c_str());
}

JNIEXPORT jstring JNICALL
native_getImei(JNIEnv* env, jobject thiz) {
    std::string pkg = getCurrentPackageName(env);
    std::string spoofed = ming::g_config.getSpoofedDeviceId(pkg);
    Logger::d("[%s] getImei() -> spoofed", pkg.c_str());
    return env->NewStringUTF(spoofed.c_str());
}

JNIEXPORT jstring JNICALL
native_getSubscriberId(JNIEnv* env, jobject thiz) {
    std::string pkg = getCurrentPackageName(env);
    std::string spoofed = ming::g_config.getSpoofedDeviceId(pkg);
    Logger::d("[%s] getSubscriberId() -> spoofed", pkg.c_str());
    return env->NewStringUTF(spoofed.c_str());
}

}

std::string getCurrentPackageName(JNIEnv* env) {
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    if (activityThreadClass) {
        jmethodID currentThread = env->GetStaticMethodID(activityThreadClass,
            "currentActivityThread", "()Landroid/app/ActivityThread;");
        if (currentThread) {
            jobject thread = env->CallStaticObjectMethod(activityThreadClass, currentThread);
            if (thread) {
                jmethodID getApp = env->GetMethodID(activityThreadClass,
                    "getApplication", "()Landroid/app/Application;");
                if (getApp) {
                    jobject app = env->CallObjectMethod(thread, getApp);
                    if (app) {
                        jclass contextClass = env->FindClass("android/content/ContextWrapper");
                        if (contextClass) {
                            jmethodID getPkg = env->GetMethodID(contextClass,
                                "getPackageName", "()Ljava/lang/String;");
                            if (getPkg) {
                                jstring pkgStr = (jstring)env->CallObjectMethod(app, getPkg);
                                if (pkgStr) {
                                    const char* pkg = env->GetStringUTFChars(pkgStr, nullptr);
                                    std::string result(pkg ? pkg : "unknown");
                                    if (pkg) env->ReleaseStringUTFChars(pkgStr, pkg);
                                    env->DeleteLocalRef(pkgStr);
                                    env->DeleteLocalRef(app);
                                    env->DeleteLocalRef(contextClass);
                                    env->DeleteLocalRef(thread);
                                    env->DeleteLocalRef(activityThreadClass);
                                    return result;
                                }
                            }
                            env->DeleteLocalRef(contextClass);
                        }
                        env->DeleteLocalRef(app);
                    }
                }
                env->DeleteLocalRef(thread);
            }
        }
        env->DeleteLocalRef(activityThreadClass);
    }

    char buf[256] = {0};
    int fd = open("/proc/self/cmdline", O_RDONLY);
    if (fd >= 0) {
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        if (n > 0 && buf[0]) return std::string(buf);
    }

    return "unknown";
}

}
