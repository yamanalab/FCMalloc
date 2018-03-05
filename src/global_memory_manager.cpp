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
#include "global_memory_manager.hpp"
#include "local_memory_manager.hpp"
#include "mem_allocate.hpp"

void GlobalMemoryManager::Init(int numCores, CommonMemoryPool& cmp, MemorySizeManager& msm)
{
    mtx_ = PTHREAD_MUTEX_INITIALIZER;

    coreN_ = numCores;
    auto poolStr = getenv("FCM_POOL_BUFFER_SIZE");
    if(poolStr == nullptr) {
        poolN_ = POOL_N;
    }
    else {
        poolN_ = atoi(poolStr);
    }

    fcmalloc::TypeAwareMemAllocate(coreN_ * poolN_, &poolFlags_);
    fcmalloc::TypeAwareMemAllocate(coreN_ * (coreN_ + 1) * poolN_, &memoryPools_);
    fcmalloc::TypeAwareMemAllocate(coreN_ * poolN_, &managerPools_);

    const int npool = coreN_ * (coreN_ + 1) * poolN_;
    for (auto i = 0; i < npool; ++i) {
        memoryPools_[i].Init(coreN_);
    }
    auto cnt = 0;
    for (auto j = 0; j < coreN_; ++j) {
        auto core = j;
        for (auto i = 0; i < poolN_; ++i) {
            poolFlags_[poolN_ * core + i] = false;
            auto& m = managerPools_[poolN_ * core + i];
            m.Init(coreN_, cmp, msm);
            m.SetCore(core);
            m.SetMalloc(&memoryPools_[cnt + 0]);
            for (auto k = 0; k < coreN_; ++k) {
                auto fcore = k;
                m.SetFree(fcore, &memoryPools_[cnt + 1 + k]);
            }
            cnt += 1 + coreN_;
        }
    }
}

LocalMemoryManager *GlobalMemoryManager::AllocLocalMemoryManager(int core)
{
    static int offset = -1;
    mtxlock l(mtx_);
    offset = (offset + 1) % poolN_;

    for (auto i = 0; i < poolN_; i++) {
        auto index = poolN_ * core + (offset + i) % poolN_;
        if (!poolFlags_[index]) {
            poolFlags_[index] = true;
            return &managerPools_[index];
        }
    }
    ASSERT(false, "Allocate more pool!\n");
    return nullptr;
}

void GlobalMemoryManager::FreeLocalMemoryManagerOtherNodeMallocLinkedList(LocalMemoryManager *m)
{
    mtxlock l(mtx_);
    auto core = m->GetCore();
    for (auto k = 0; k < coreN_; ++k) {
        if (k != core) {
            for (auto ii = 0; ii < poolN_; ++ii) {
                if (!poolFlags_[poolN_ * k + ii]) {
                    managerPools_[poolN_ * k + ii].Join(k, m);
                }
            }
        }
    }
    return;
}

void GlobalMemoryManager::FreeLocalMemoryManager(LocalMemoryManager *m)
{
    mtxlock l(mtx_);
    static int offset = -1;
    offset = (offset + 1) % coreN_;
    for (auto j = 0; j < coreN_; ++j) {
        auto core = (j + offset) % coreN_;
        for (auto i = 0; i < poolN_; ++i) {
            if (poolFlags_[poolN_ * core + i] && &managerPools_[poolN_ * core + i] == m) {
                for (auto k = 0; k < coreN_; ++k) {
                    for (auto ii = 0; ii < poolN_; ++ii) {
                        if (i == ii) {
                            continue;
                        }
                        if (!poolFlags_[poolN_ * k + ii]) {
                            managerPools_[poolN_ * k + ii].Join(k, m);
                        }
                    }
                }
                poolFlags_[poolN_ * core + i] = false;
                return;
            }
        }
    }
    ASSERT(false, "no alloced LocalMemoryManager!\n");
    return;
}

void GlobalMemoryManager::Free(void *ptr)
{
    auto core = MemUtil::PtrToCore(ptr);
    auto lp = AllocLocalMemoryManager(core);
    lp->Free(ptr);
    FreeLocalMemoryManager(lp);
}
