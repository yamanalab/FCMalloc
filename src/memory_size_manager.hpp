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

#include "common.hpp"

class MemorySizeManager
{
    public:
        MemorySizeManager();
        MemorySizeManager(const MemorySizeManager&) = delete;
        MemorySizeManager& operator=(const MemorySizeManager&) = delete;
        ~MemorySizeManager();

        int GetMemorySize(int log2_size) const
        {
            ASSERT(0 <= log2_size && log2_size < Size, "log2_size is out of range\n");
            return nPerSize_[log2_size];
        }

        static const int Size = 64 + 1;

    private:
        void setDefault();
        void readSizeListFile(const char* filename);

        int nPerSize_[Size];
};

