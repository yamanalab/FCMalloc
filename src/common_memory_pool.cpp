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

#include "common_memory_pool.hpp"
#include "memory_linked_list_manager.hpp"
#include "memory_size_manager.hpp"

#include "mem_allocate.hpp"

void CommonMemoryPool::Init(const int numCores, MemorySizeManager& msm)
{
    coreN_ = numCores;

    msm_ = &msm;

    fcmalloc::TypeAwareMemAllocate(coreN_, &mtxsPerCore_);
    fcmalloc::TypeAwareMemAllocate(coreN_, &poolsPerCore_);

    for (auto i = 0; i < coreN_; i++) {
        mtxsPerCore_[i] = PTHREAD_MUTEX_INITIALIZER;
    }
}

void *CommonMemoryPool::Malloc(MemoryLinkedListManager *mllm, int core, size_t size)
{
    if (size == 0) {
        return nullptr;
    }
    ASSERT(((0 <= core) && (core < coreN_)), "core = %d\n", core);

    auto index = logarithm2(size);
    //  NOTE the number of pooled memory chunks differs depending on size
    auto n = msm_->GetMemorySize(index);
    ASSERT(n > 0, "Please cahnge n per size! size = %ld, 2^x(x=%d)\n", size, index);

    {
        mtxlock l(mtxsPerCore_[core]);
        for (auto i = 0; i < n; i++) {
            MemoryLinkedList *newHead = poolsPerCore_[core].pop(index);
            if (newHead == nullptr) {
                break;
            }
            mllm->push(index, newHead);
        }
    }

    return mllm->Malloc(size);
}

void CommonMemoryPool::Free(MemoryLinkedListManager *mllm, int core)
{
    ASSERT(((0 <= core) && (core < coreN_)), "core = %d\n", core);

    mtxlock l(mtxsPerCore_[core]);
    poolsPerCore_[core].Join(mllm);
}
