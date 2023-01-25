
/*
 * Reference: https://github.com/joyeecheung/md5
 */
#ifndef _MD5_H
#define _MD5_H
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <array>
#include <iterator>
#include <cstdint>
#define BLOCK_SIZE 64  // 16-word referece RFC 1321 3.4
class MD5 {
public:
    MD5();
    MD5& update(const unsigned char* in, size_t inputLen);
    MD5& update(const char* in, size_t inputLen);
    MD5& finalize();
    std::string toString() const;

private:
    void init();
    void transform(const uint8_t block[BLOCK_SIZE]);

    uint8_t buffer[BLOCK_SIZE];  // buffer of the raw data
    uint8_t digest[16];  // result hash, little endian

    uint32_t state[4];  // state (ABCD)
    uint32_t lo, hi;    // number of bits, modulo 2^64 (lsb first)
    bool finalized;  // if the context has been finalized
};

std::string md5(const std::string str);

#endif