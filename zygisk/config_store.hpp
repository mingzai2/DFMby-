#pragma once

#include <string>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctime>

#include "sha256_util.hpp"

namespace ming {

class ConfigStore {
private:
    static constexpr const char* DEFAULT_MOD_DIR = "/data/adb/modules/mingdfm";
    static constexpr const char* SEED_FILE = "tools/drm_seed";

    std::string modDir;
    std::string seed;
    std::mutex mtx;

    std::string readFile(const std::string& path) {
        int fd = open(path.c_str(), O_RDONLY);
        if (fd < 0) return "";
        char buf[2048];
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        if (n <= 0) return "";
        buf[n] = '\0';
        std::string s(buf);
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) {
            s.pop_back();
        }
        return s;
    }

    void writeFile(const std::string& path, const std::string& content) {
        int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) return;
        write(fd, content.c_str(), content.size());
        close(fd);
    }

    std::string generateSeed() {
        unsigned char rand_bytes[32];
        int fd = open("/dev/urandom", O_RDONLY);
        if (fd >= 0) {
            ssize_t r = read(fd, rand_bytes, 32);
            if (r != 32) {
                for (int i = 0; i < 32; i++) rand_bytes[i] = (unsigned char)(i * 37 + getpid());
            }
            close(fd);
        } else {
            srand((unsigned int)(getpid() ^ time(NULL)));
            for (int i = 0; i < 32; i++) {
                rand_bytes[i] = (unsigned char)((rand() ^ getpid() ^ (i * 7)) & 0xFF);
            }
        }

        char hex[65];
        for (int i = 0; i < 32; i++) {
            sprintf(hex + i * 2, "%02x", rand_bytes[i]);
        }
        hex[64] = '\0';
        return std::string(hex);
    }

public:
    ConfigStore() : modDir(DEFAULT_MOD_DIR) {}

    void init(const std::string& moduleDir) {
        std::lock_guard<std::mutex> lock(mtx);
        if (!moduleDir.empty()) {
            modDir = moduleDir;
        }

        std::string seedPath = modDir + "/" + SEED_FILE;
        seed = readFile(seedPath);

        if (seed.empty() || seed.size() < 32) {
            seed = generateSeed();
            writeFile(seedPath, seed);
        }
    }

    std::string getSeed() {
        std::lock_guard<std::mutex> lock(mtx);
        return seed;
    }

    std::string getSpoofedDrmId(const std::string& packageName) {
        std::lock_guard<std::mutex> lock(mtx);
        std::string input = seed + ":" + packageName + ":drm_unique_id_v1";
        return crypto::sha256Hex(input);
    }

    std::string getSpoofedDeviceId(const std::string& packageName) {
        std::lock_guard<std::mutex> lock(mtx);
        std::string input = seed + ":" + packageName + ":device_id_v1";
        std::string hash = crypto::sha256Hex(input);
        return hash.substr(0, 16);
    }

    std::string getSpoofedAndroidId(const std::string& packageName) {
        std::lock_guard<std::mutex> lock(mtx);
        std::string input = seed + ":" + packageName + ":android_id_v1";
        std::string hash = crypto::sha256Hex(input);
        return hash.substr(0, 16);
    }

    std::string getModuleDir() {
        return modDir;
    }
};

}
