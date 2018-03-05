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

#include "local_memory_manager.hpp"
#include "common_memory_pool.hpp"
#include "memory_size_manager.hpp"

#include <string.h>

void LocalMemoryManager::Init(int numCores, CommonMemoryPool& cmp, MemorySizeManager& msm) {
    core_ = 0;
    coreN_ = numCores;
    malloc_ = nullptr;
    fcmalloc::TypeAwareMemAllocate(coreN_, &free_);
    cmp_ = &cmp;
    msm_ = &msm;
}

//  NOTE index is log2(value)
void LocalMemoryManager::swap(int index)
{
    ASSERT(free_[core_] != nullptr, "free list [core] is nullptr\n");
    malloc_->Swap(free_[core_], index);
}

void* LocalMemoryManager::Malloc(size_t size)
{
    if (size == 0) {
        return nullptr;
    }
    ASSERT(malloc_ != nullptr, "malloc list is nullptr\n");

    auto ptr = malloc_->Malloc(size);
    if (ptr == nullptr) {
        auto index = logarithm2(size);
        swap(index);
        ptr = malloc_->Malloc(size);
        if (ptr == nullptr) {
            ptr = cmp_->Malloc(malloc_, core_, size);
            if (ptr == nullptr) {
                auto n = msm_->GetMemorySize(index);
                malloc_->Allocate(core_, size, n);
                ptr = malloc_->Malloc(size);
                if (ptr == nullptr) {
                    errno = ENOMEM;
                }
            }
        }
    }
#ifdef DEBUG
    InclCounter(ptr, size, true);
#endif
    return ptr;
}

void* LocalMemoryManager::Realloc(void* ptr, size_t size)
{
    if (ptr == nullptr) {
        return Malloc(size);
    }

    auto m = (MemoryLinkedList *)((uintptr_t)ptr - sizeof(MemoryLinkedList));
    m->Assert();
    auto preSize = m->GetBodySize();
    if (size <= preSize) {
        return ptr;
    }

    auto newPtr = Malloc(size);
    if(newPtr != nullptr) {
        memcpy(newPtr, ptr, preSize);
        Free(ptr);
        return newPtr;
    }
    else {
        return nullptr;
    }
}

void LocalMemoryManager::Free(void* ptr)
{
    if (ptr == nullptr) {
        return;
    }
    ASSERT(free_ != nullptr, "free list is nullptr\n");
    ASSERT(free_[core_] != nullptr, "free list [core] is nullptr\n");

    auto core = MemUtil::PtrToCore(ptr);
    free_[core]->Free(ptr);
#ifdef DEBUG
    InclCounter(ptr, MemUtil::PtrToSize(ptr), false);
#endif
}

void LocalMemoryManager::AllFreeToCommonMemoryPool()
{
    ASSERT(free_ != nullptr, "free list is nullptr\n");
    ASSERT(free_[core_] != nullptr, "free list [core] is nullptr\n");

    //  remote memory -> common memory pool
    for (auto i = 0; i < coreN_; ++i) {
        if (i != core_) {
            cmp_->Free(free_[i], i);
        }
    }
}

void LocalMemoryManager::Join(int core, LocalMemoryManager* lm)
{
    auto lmFree = lm->free_[core];
    malloc_->Join(lmFree);
}

namespace MemUtil {
    MemoryLinkedList* getList(uintptr_t ptr)
    {
        auto m = (MemoryLinkedList*)(ptr - sizeof(MemoryLinkedList));
        ASSERT(m->CheckSignature(), "previous memory size is unknown\n");
        m->Assert();
        return m;
    }
    size_t PtrToSize(void* ptr)
    {
        auto m = getList((uintptr_t)ptr);
        size_t size = m->GetBodySize();
        return size;
    }
    int PtrToCore(void* ptr)
    {
        auto m = getList((uintptr_t)ptr);
        auto core = m->GetCore();
        return core;
    }
}
