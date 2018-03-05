/*
 * Copyright 2017 Yamana Laboratory, Waseda University
 * Supported by JST CREST Grant Number JPMJCR1503, Japan.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <unistd.h> // for size_t

class mtxlock
{
    public:
        explicit mtxlock(pthread_mutex_t& mutex)
            : mutex_(mutex)
        {
            int r = pthread_mutex_lock(&mutex_);
            if(r != 0) {
                fprintf(stderr, "%s: %s\n", __func__, strerror(r));
                assert(0);
            }
        }
        ~mtxlock()
        {
            int r = pthread_mutex_unlock(&mutex_);
            if(r != 0) {
                fprintf(stderr, "%s: %s\n", __func__, strerror(r));
                assert(0);
            }
        }

    private:
        pthread_mutex_t& mutex_;
};

#define SIGNATURE (0xDEADC0DE)
#define ALIGN(v, n) (((((uintptr_t)v) + (n) - 1) / (n)) * (n))
#define ALIGN_REMAIN(ptr, aligenment) ((uintptr_t)(ptr) % (aligenment))
#define ALIGN_CHECK(ptr, aligenment) (((ptr) != 0) || ((uintptr_t)(ptr) % (aligenment)) == 0)

inline size_t roundup_powerof2(size_t x)
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return ++x;
}

inline size_t logarithm2(size_t x)
{
    if(x <= 8) {
        // min size must be 8 = 2^3 byte, so this return 3
        return 3;
    }
    else if (x >= 0x8000000000000000) {
        return 64;
    }

#if 0
    size_t v = 1;
    size_t i;
    for (i = 0; v < x; i++) {
        v *= 2;
    }
    // min size must be 8 = 2^3 byte, so this return 3
    if (i < 3) {
        i = 3;
    }
    return i;
#else
    const size_t v = x - (x > 1);
    // low
    // NOTE:__builtin_clz
    // clang:__builtin_clz(0)=31
    // g++:__builtin_clz(0)=32
    if (x < (1UL << 32)) return 32 - __builtin_clz(v);
    // high
    return 32 + (32 - __builtin_clz(v >> 32));
#endif
}

inline bool multCausesOverflow(size_t a, size_t b)
{
    auto x = a * b;
    return (a != 0 && x / a != b);
}

inline bool addCausesOverflow(size_t a, size_t b)
{
    auto max_size = (size_t)-1;
    return (max_size - a < b);
}

#define OVERFLOW_MUL_ASSERT(a, b) ASSERT(!multCausesOverflow((a), (b)), "mul. overflow error: a = %ld, b = %ld\n", (a), (b));
#define OVERFLOW_ADD_ASSERT(a, b) ASSERT(!addCausesOverflow((a), (b)), "add. overflow error: a = %ld, b = %ld\n", (a), (b));
