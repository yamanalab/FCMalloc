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

#include "memory_size_manager.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

MemorySizeManager::MemorySizeManager()
{
    memset(nPerSize_, 0, Size * sizeof(int));

    auto eval = getenv("FCM_SIZE_LIST_FILE");
    if(eval == nullptr) {
        setDefault();
    }
    else {
        readSizeListFile(eval);
    }
}

MemorySizeManager::~MemorySizeManager()
{
    // currently do nothing
}

void MemorySizeManager::setDefault()
{
    nPerSize_[0]  = 0;      //  ~1B
    nPerSize_[1]  = 0;      //  ~2B
    nPerSize_[2]  = 0;      //  ~4B
    nPerSize_[3]  = 1000;   //  ~8B
    nPerSize_[4]  = 2000;   //  ~16B
    nPerSize_[5]  = 2000;   //  ~32B
    nPerSize_[6]  = 10000;  //  ~64B
    nPerSize_[7]  = 1000;   //  ~128B
    nPerSize_[8]  = 1000;   //  ~256B
    nPerSize_[9]  = 1000;   //  ~512B
    nPerSize_[10] = 1024;  //  ~1KB
    nPerSize_[11] = 512;   //  ~2KB
    nPerSize_[12] = 512;   //  ~4KB
    nPerSize_[13] = 1024;  //  ~8KB
    nPerSize_[14] = 10240; //  ~16KB
    nPerSize_[15] = 128;   //  ~32KB
    nPerSize_[16] = 2;     //  ~64KB
    nPerSize_[17] = 1;     //  ~128KB
    nPerSize_[18] = 1;     //  ~256KB
    nPerSize_[19] = 1;     //  ~512KB
    nPerSize_[20] = 1;     //  ~1MB
    nPerSize_[21] = 1;     //  ~2MB
    nPerSize_[22] = 1;     //  ~4MB
    nPerSize_[23] = 1;     //  ~8MB
    nPerSize_[24] = 1;     //  ~16MB
    nPerSize_[25] = 1;     //  ~32MB
    nPerSize_[26] = 1;     //  ~64MB
    nPerSize_[27] = 1;     //  ~128MB
    nPerSize_[28] = 1;     //  ~256MB
    nPerSize_[29] = 1;     //  ~512MB
    nPerSize_[30] = 1;     //  ~1GB
    nPerSize_[31] = 1;     //  ~2GB
    nPerSize_[32] = 1;     //  ~4GB
    nPerSize_[33] = 1;     //  ~8GB
    nPerSize_[34] = 1;     //  ~16GB
    nPerSize_[35] = 0;     //  ~32GB
    nPerSize_[36] = 0;     //  ~64GB
    nPerSize_[37] = 0;     //  ~128GB
    nPerSize_[38] = 0;     //  ~256GB
}

void MemorySizeManager::readSizeListFile(const char* filename)
{
    const int max_num_digits = 5;
    char val[Size * max_num_digits + 1];
    int fd = open(filename, O_RDONLY);
    if(fd != -1) {
        memset(val, 0, sizeof(val));
        auto index = 0;
        while(read(fd, val, sizeof(val)) > 0) {
            auto token = strtok(val, "\n");
            while(token != nullptr && index < Size) {
                nPerSize_[index++] = atoi(token);
                token = strtok(nullptr, "\n");
            }
        }
        close(fd);
    }
    else {
        setDefault();
    }
}

