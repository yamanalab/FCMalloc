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

#ifdef DEBUG
#define Debug(fmt, ...) \
    myprintf("[DEBUG][%s] %s:%u # " fmt "", __DATE__, __FILE__, __LINE__, ##__VA_ARGS__);
#else
#define Debug(fmt, ...)
#endif

#define Info(fmt, ...) \
    myprintf("[INFO ][%s] %s:%u # " fmt "", __DATE__, __FILE__, __LINE__, ##__VA_ARGS__);

#define Warn(fmt, ...) \
    myprintf("[WARN ][%s] %s:%u # " fmt "", __DATE__, __FILE__, __LINE__, ##__VA_ARGS__);

#define Log(fmt, ...) \
    myprintf("[LOG  ][%s] %s:%u # " fmt "", __DATE__, __FILE__, __LINE__, ##__VA_ARGS__);

#ifdef DEBUG
#define ASSERT(flag, fmt, ...)                                                 \
    {                                                                          \
        bool ____ret = (flag);                                                 \
        if (!____ret) {                                                        \
            myprintf("%20s:%4d # " fmt "", __FILE__, __LINE__, ##__VA_ARGS__); \
            fflush(stdout);                                                    \
            abort();                                                           \
        }                                                                      \
    }
#else
#define ASSERT(flag, fmt, ...)
#endif
