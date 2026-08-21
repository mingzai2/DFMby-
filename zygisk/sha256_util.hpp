#pragma once

#include <string>
#include <cstdint>
#include <cstring>
#include <cstdio>

namespace ming {
namespace crypto {

class SHA256 {
private:
    uint32_t H[8];
    uint8_t M[64];
    uint64_t length;
    int curlen;
    bool computed;

    static constexpr uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    static inline uint32_t RORc(uint32_t x, int y) {
        return (x >> y) | (x << (32 - y));
    }

    static inline uint32_t Ch(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (~x & z);
    }

    static inline uint32_t Maj(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (x & z) ^ (y & z);
    }

    static inline uint32_t Sigma0(uint32_t x) {
        return RORc(x, 2) ^ RORc(x, 13) ^ RORc(x, 22);
    }

    static inline uint32_t Sigma1(uint32_t x) {
        return RORc(x, 6) ^ RORc(x, 11) ^ RORc(x, 25);
    }

    static inline uint32_t Gamma0(uint32_t x) {
        return RORc(x, 7) ^ RORc(x, 18) ^ (x >> 3);
    }

    static inline uint32_t Gamma1(uint32_t x) {
        return RORc(x, 17) ^ RORc(x, 19) ^ (x >> 10);
    }

    void compress(const uint8_t* buf) {
        uint32_t S[8], W[64];
        int i;

        for (i = 0; i < 8; i++) S[i] = H[i];

        for (i = 0; i < 16; i++) {
            W[i] = ((uint32_t)buf[i*4] << 24) |
                   ((uint32_t)buf[i*4+1] << 16) |
                   ((uint32_t)buf[i*4+2] << 8) |
                   ((uint32_t)buf[i*4+3]);
        }
        for (; i < 64; i++) {
            W[i] = Gamma1(W[i-2]) + W[i-7] + Gamma0(W[i-15]) + W[i-16];
        }

        for (i = 0; i < 64; i++) {
            uint32_t t0 = S[7] + Sigma1(S[4]) + Ch(S[4], S[5], S[6]) + K[i] + W[i];
            uint32_t t1 = Sigma0(S[0]) + Maj(S[0], S[1], S[2]);
            S[7] = S[6];
            S[6] = S[5];
            S[5] = S[4];
            S[4] = S[3] + t0;
            S[3] = S[2];
            S[2] = S[1];
            S[1] = S[0];
            S[0] = t0 + t1;
        }

        for (i = 0; i < 8; i++) H[i] += S[i];
    }

public:
    SHA256() : length(0), curlen(0), computed(false) {
        H[0] = 0x6a09e667; H[1] = 0xbb67ae85;
        H[2] = 0x3c6ef372; H[3] = 0xa54ff53a;
        H[4] = 0x510e527f; H[5] = 0x9b05688c;
        H[6] = 0x1f83d9ab; H[7] = 0x5be0cd19;
    }

    void update(const uint8_t* data, size_t len) {
        if (computed) return;
        while (len--) {
            M[curlen++] = *data++;
            if (curlen == 64) {
                compress(M);
                length += 512;
                curlen = 0;
            }
        }
    }

    void update(const std::string& s) {
        update((const uint8_t*)s.c_str(), s.size());
    }

    void finalize(uint8_t* out) {
        if (!computed) {
            length += curlen * 8;
            M[curlen++] = 0x80;
            if (curlen > 56) {
                while (curlen < 64) M[curlen++] = 0;
                compress(M);
                curlen = 0;
            }
            while (curlen < 56) M[curlen++] = 0;
            for (int i = 0; i < 8; i++) {
                M[56 + i] = (uint8_t)(length >> (56 - i * 8));
            }
            compress(M);
            computed = true;
        }

        for (int i = 0; i < 8; i++) {
            out[i*4]   = (uint8_t)(H[i] >> 24);
            out[i*4+1] = (uint8_t)(H[i] >> 16);
            out[i*4+2] = (uint8_t)(H[i] >> 8);
            out[i*4+3] = (uint8_t)(H[i]);
        }
    }

    std::string hexDigest() {
        uint8_t hash[32];
        finalize(hash);
        char hex[65];
        for (int i = 0; i < 32; i++) {
            sprintf(hex + i * 2, "%02x", hash[i]);
        }
        hex[64] = '\0';
        return std::string(hex);
    }
};

inline std::string sha256Hex(const std::string& input) {
    SHA256 sha;
    sha.update(input);
    return sha.hexDigest();
}

}
}
