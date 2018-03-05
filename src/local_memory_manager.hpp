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
#include "mem_allocate.hpp"
#include "memory_linked_list_manager.hpp"

#ifdef DEBUG
#include <ctime>
#include <fcntl.h>
// if you set `SIZE_BASED_LOG` 1, size-based log is enabled
#define SIZE_BASED_LOG 0
#endif

class CommonMemoryPool;
class MemorySizeManager;

class LocalMemoryManager {
    public:
        void Init(int numCores, CommonMemoryPool& cmp, MemorySizeManager& msm);

        void SetMalloc(MemoryLinkedListManager* malloc)
        {
            malloc_ = malloc;
        }
        void SetFree(int core, MemoryLinkedListManager* free)
        {
            ASSERT(((0 <= core) && (core < coreN_)), "core = %d\n", core);
            free_[core] = free;
        }
        void SetCore(int core)
        {
            ASSERT(((0 <= core) && (core < coreN_)), "core = %d\n", core);
            core_ = core;
#ifdef DEBUG
            ResetCounter();
#endif
        }

        int GetCore() const
        {
            return core_;
        }

        void *Malloc(size_t size);
        void *Realloc(void *ptr, size_t size);
        void Free(void *ptr);
        void AllFreeToCommonMemoryPool();

        size_t GetAllFreeLength() const
        {
            auto cnt = 0u;
            for (auto i = 0; i < coreN_; ++i) {
                cnt += free_[i]->GetAllFreeLength();
            }
            return cnt;
        }

        void Join(int core, LocalMemoryManager* lm);

    private:
        void swap(int index);

#ifdef DEBUG
#if SIZE_BASED_LOG
        void ResetCounter()
        {
            if(!numMalloc_) {
                fcmalloc::TypeAwareMemAllocate(MemorySizeManager::Size, &numMalloc_);
            }
            memset(numMalloc_, 0, MemorySizeManager::Size * sizeof(size_t));
            if(!numFree_) {
                fcmalloc::TypeAwareMemAllocate(MemorySizeManager::Size, &numFree_);
            }
            memset(numFree_, 0, MemorySizeManager::Size * sizeof(size_t));
            outIntvl_ = 1;
            outFd_ = fdopen(core_);
        }
        void InclCounter(void* ptr, size_t size, bool isAlloc)
        {
            if(ptr) {
                auto index = logarithm2(size);
                auto counter = ((isAlloc) ? numMalloc_ : numFree_) + index;
                if(++*counter % outIntvl_ == 0) {
                    auto now = (size_t)time(nullptr);
                    myprintf(outFd_, "%u [%3d] size: %5u, #malloc: %5u, #free: %5u\n", now, core_, (1u << index), numMalloc_[index], numFree_[index]);
                }
            }
        }

        size_t* numMalloc_;
        size_t* numFree_;
        size_t outIntvl_;
        int outFd_;
#else
        void ResetCounter()
        {
            numMalloc_ = 0;
            numFree_ = 0;
            auto intvlStr = getenv("FCM_MEM_LOG_INTVL");
            outIntvl_ = (intvlStr == nullptr) ? 10000 : atoi(intvlStr);
            outFd_ = fdopen(core_);
        }
        void InclCounter(void* ptr, size_t size, bool isAlloc)
        {
            if(ptr) {
                auto& counter = (isAlloc) ? numMalloc_ : numFree_;
                if(++counter % outIntvl_ == 0) {
                    auto now = (size_t)time(nullptr);
                    myprintf(outFd_, "%u [%3d] #malloc: %5u, #free: %5u\n", now, core_, numMalloc_, numFree_);
                }
            }
        }

        size_t numMalloc_;
        size_t numFree_;
        size_t outIntvl_;
        int outFd_;
#endif // SIZE_BASED_LOG
#endif // DEBUG

        int core_;
        int coreN_;

        MemoryLinkedListManager* malloc_;
        MemoryLinkedListManager** free_;

        CommonMemoryPool* cmp_;
        MemorySizeManager* msm_;
};

namespace MemUtil {
    size_t PtrToSize(void* ptr);
    int PtrToCore(void* ptr);
}
