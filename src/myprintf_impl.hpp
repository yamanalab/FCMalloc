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

#include "myprintf.hpp"

#include <cstdint>
#include <unistd.h>

const int stdout_fd = 1;
const int stderr_fd = 2;

const int size = 128;

void repeat_char(int fd, char c, int n);
void mywrite(int fd, const char* const cstr);
void mywrite(int fd, const char* s, const char* x, int num = 0);
void mywrite(int fd, const char* s, const int& x, int num = 0);
void mywrite(int fd, const char* s, const unsigned int& x, int num = 0);
void mywrite(int fd, const char* s, const long unsigned int& x, int num = 0);
void mywrite(int fd, const char* s, const unsigned char& x, int num = 0);

template <typename T>
void convDec(char*& pbuf, T val)
{
    if (val == 0) {
        *(--pbuf) = '0';
    }
    while (val > 0) {
        *(--pbuf) = '0' + (val % 10);
        val /= 10;
    }
}

template <typename T>
void convHex(char*& pbuf, T val)
{
    if (val == 0) {
        *(--pbuf) = '0';
    }
    while (val > 0) {
        auto v = val & 0x0f;
        *(--pbuf) = (v < 10) ? '0' + v : 'a' + (v - 10);
        val >>= 4;
    }
    *(--pbuf) = 'x';
    *(--pbuf) = '0';
}

template <typename T>
void conv(char fmt, char*& pbuf, T val)
{
    switch(fmt) {
        case 'd':
        case 'u':
            convDec(pbuf, val);
            break;
        case 'x':
            convHex(pbuf, val);
            break;
    }
}

template <typename T>
inline void mywrite(int fd, const char* s, const T* x, int num = 0)
{
    auto ptr_val = (uintptr_t)x;
    char buf[size] = { 0 };
    const auto pbuf_end = buf + size;
    auto pbuf = pbuf_end;
    convHex(pbuf, ptr_val);
    write(fd, (void*)pbuf, pbuf_end - pbuf);
    repeat_char(fd, ' ', num - (pbuf_end - pbuf));
}

int fdopen(int index = -1);

void myprintf(int fd, const char* s);

template <typename T, typename... Args>
inline void myprintf(int fd, const char* s, const T& value, const Args&... args)
{
    auto ps_begin = (char*)s;
    auto ps = ps_begin;
    while (*ps) {
        if (*ps == '%') {
            write(fd, (void*)ps_begin, ps - ps_begin);
            ps_begin = ++ps;

            int num = 0;
            while ('0' <= *ps && *ps <= '9') {
                num *= 10;
                num += *ps - '0';
                ++ps;
            }
            // ignore `l` of %ld, %lu, %lld, or %llu
            if(*ps == 'l') {
                ++ps;
                if(*ps == 'l') {
                    ++ps;
                }
            }
            if (*ps != '%') {
                mywrite(fd, ps, value, num);
                ++ps;
                myprintf(fd, ps, args...);
                return;
            }
        }
        ++ps;
    }
}

template <typename T, typename... Args>
inline void myprintf(const char* s, const T& value, const Args&... args)
{
    auto fd = fdopen();
    if (fd <= 0) {
        return;
    }
    myprintf(fd, s, value, args...);
}
