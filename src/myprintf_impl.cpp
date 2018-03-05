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

#include "myprintf_impl.hpp"

#include <cstring>
#include <cstdlib>
#include <fcntl.h>

void repeat_char(int fd, char c, int n)
{
    if (n <= 0) {
        return;
    }
    char s[size] = { 0 };
    memset((void*)s, c, size);
    for(auto i = 0; i < n / size; ++i) {
        write(fd, (void*)s, size);
    }
    auto remain = n % size;
    if(remain > 0) {
        write(fd, (void*)s, remain);
    }
}

void mywrite(int fd, const char* const cstr)
{
    write(fd, (void*)cstr, strlen(cstr));
}

void mywrite(int fd, const char* s, const char* x, int num)
{
    auto len = strlen(x);
    repeat_char(fd, ' ', num - len);
    write(fd, (void*)x, len);
}

void mywrite(int fd, const char* s, const int& x, int num)
{
    char buf[size] = { 0 };
    const auto pbuf_end = buf + size;
    auto pbuf = pbuf_end;
    auto xval = x;
    auto revFlag = (xval < 0);
    if (revFlag) {
        xval = -xval;
    }
    conv(*s, pbuf, xval);
    if (revFlag) {
        *(--pbuf) = '-';
    }
    repeat_char(fd, ' ', num - (pbuf_end - pbuf));
    write(fd, (void*)pbuf, pbuf_end - pbuf);
}

void mywrite(int fd, const char* s, const unsigned int& x, int num)
{
    char buf[size] = { 0 };
    const auto pbuf_end = buf + size;
    auto pbuf = pbuf_end;
    conv(*s, pbuf, x);
    repeat_char(fd, ' ', num - (pbuf_end - pbuf));
    write(fd, (void*)pbuf, pbuf_end - pbuf);
}

void mywrite(int fd, const char* s, const long unsigned int& x, int num)
{
    char buf[size] = { 0 };
    const auto pbuf_end = buf + size;
    auto pbuf = pbuf_end;
    conv(*s, pbuf, x);
    repeat_char(fd, ' ', num - (pbuf_end - pbuf));
    write(fd, (void*)pbuf, pbuf_end - pbuf);
}

// only unsigned char shown in hex
void mywrite(int fd, const char* s, const unsigned char& x, int num)
{
    char buf[size] = { 0 };
    const auto pbuf_end = buf + size;
    auto pbuf = pbuf_end;
    conv(*s, pbuf, x);
    repeat_char(fd, ' ', num - (pbuf_end - pbuf));
    write(fd, (void*)pbuf, pbuf_end - pbuf);
}

namespace {
    int main_fd = 0;
    int fdopen_main()
    {
        if(main_fd == 0) {
            auto filename = getenv("FCM_LOG_OUTPUT");
            auto isConsole = false;
            if(filename == nullptr) {
                filename = (char*)"fcmalloc.log";
            }
            else if(strcmp(filename, "stdout") == 0) {
                main_fd = stdout_fd;
                isConsole = true;
            }
            else if(strcmp(filename, "stderr") == 0) {
                main_fd = stderr_fd;
                isConsole = true;
            }
            else if (strcmp(filename, "/dev/null") == 0) {
                main_fd = -1;
                isConsole = true;
            }

            if(!isConsole) {
                main_fd = open(filename, O_RDWR | O_CREAT | O_APPEND,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if(main_fd == -1) {
                    mywrite(stderr_fd, "failed to open %s\n", filename);
                }
            }
        }
        return main_fd;
    }

    int fdopen_mt(int index)
    {
        auto prefix = getenv("FCM_LOG_PREFIX");
        auto postfix = ".log";
        const auto min_digit = 3;
        char filename[size] = { 0 };
        if(prefix == nullptr) {
            prefix = (char*)"fcmalloc";
        }
        // construct filename `prefix_xxx.log`
        auto p = filename;
        auto prelen = strlen(prefix);
        memcpy(p, prefix, prelen * sizeof(char));
        p += prelen;
        *(p++) = '_';
        auto val = index;
        auto numlen = (val == 0) ? 1 : 0;
        while(val > 0) {
            ++numlen;
            val /= 10;
        }
        val = min_digit - numlen;
        while(val-- > 0) {
            *(p++) = '0';
        }
        p += numlen;
        convDec(p, index);
        memcpy(p + numlen, postfix, strlen(postfix) * sizeof(char));
        auto fd = open(filename, O_RDWR | O_CREAT | O_APPEND,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if(fd == -1) {
            mywrite(stderr_fd, "failed to open %s\n", filename);
        }
        return fd;
    }
}


int fdopen(int index)
{
    return (index == -1) ? fdopen_main() : fdopen_mt(index);
}

void myprintf(int fd, const char* s)
{
    auto ps_begin = (char*)s;
    auto ps = ps_begin;
    while (*ps) {
        if (*ps == '%') {
            write(fd, (void*)ps_begin, ps - ps_begin);
            ps_begin = ++ps;
        }
        ++ps;
    }
    write(fd, (void*)ps_begin, ps - ps_begin);
}
