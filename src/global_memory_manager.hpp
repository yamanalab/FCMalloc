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

class CommonMemoryPool;
class LocalMemoryManager;
class MemoryLinkedListManager;
class MemorySizeManager;

// default pool size
#define POOL_N 4

class GlobalMemoryManager {
    public:
        void Init(int numCores, CommonMemoryPool& cmp, MemorySizeManager& msm);
        LocalMemoryManager *AllocLocalMemoryManager(int core);
        void FreeLocalMemoryManager(LocalMemoryManager *m);
        void FreeLocalMemoryManagerOtherNodeMallocLinkedList(LocalMemoryManager *m);
        void Free(void *ptr);

    private:
        pthread_mutex_t mtx_;

        int coreN_;
        int poolN_;

        bool* poolFlags_;
        MemoryLinkedListManager* memoryPools_;
        LocalMemoryManager* managerPools_;
};
