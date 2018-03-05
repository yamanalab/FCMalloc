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

#include "mmap_manager.hpp"

#include "mem_allocate.hpp"

#include <cerrno>
#include <sys/mman.h>

using namespace std;

const size_t oneMB = 1024 * 1024;
const size_t oneGB = 1024 * 1024 * 1024;

extern thread_local bool mainThreadFlag;

void MmapManager::Init(int numCores, size_t pageSize, size_t mainSize, size_t subTotalSize)
{
    coreN_ = numCores;
    pageSize_ = pageSize;

    threadN_ = coreN_ + 1;
    mainThreadIndex_ = threadN_ - 1;

    // the following parameter has been determined by experiment
    extendMax_ = 10000;

    fcmalloc::TypeAwareMemAllocate(threadN_, &mtxs_);
    fcmalloc::TypeAwareMemAllocate(threadN_, &firstTouchFlag_);
    fcmalloc::TypeAwareMemAllocate(threadN_, &sizes_);
    fcmalloc::TypeAwareMemAllocate(threadN_, &offsets_);
    fcmalloc::TypeAwareMemAllocate(threadN_, &pools_);
    fcmalloc::TypeAwareMemAllocate(threadN_ * extendMax_, &sizess_);
    fcmalloc::TypeAwareMemAllocate(threadN_ * extendMax_, &poolss_);
    fcmalloc::TypeAwareMemAllocate(threadN_, &debugSizes_);
    fcmalloc::TypeAwareMemAllocate(threadN_, &debugOffsets_);

    auto mainMmapSize = ALIGN(mainSize, pageSize_);
    auto subMmapSize  = ALIGN(subTotalSize / coreN_, pageSize_);

    auto totalMmapSize = mainMmapSize + subMmapSize * coreN_;
    #if 0 // if sysconf below worked properly, this `if` should be enabled
    auto numPages = sysconf(_SC_PHYS_PAGES);
    auto memSize = (unsigned long long)pageSize_ * numPages;
    ASSERT(totalMmapSize <= memSize, "req: size < %ld :totalMmapSize = %ld\n", memSize, totalMmapSize);
    #else
    ASSERT(totalMmapSize <= 1024 * 1024 * 1024 * 1024UL, "req: size < 1TB :totalMmapSize = %ld\n", totalMmapSize);
    #endif

    auto devZero = -1;
    for (auto i = 0; i < threadN_; i++) {
        auto mmapSize = (i == mainThreadIndex_) ? mainMmapSize : subMmapSize;
        auto p = (void *)mmap(nullptr, mmapSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, devZero, 0);
        ASSERT((p != (void *)-1), "mmap result is -1: errno=%d\n", errno);
        pools_[i] = p;
        sizes_[i] = mmapSize;
        offsets_[i] = 0;
        debugSizes_[i] = mmapSize;
        debugOffsets_[i] = 0;

        sizess_[extendMax_ * i + 0] = sizes_[i];
        poolss_[extendMax_ * i + 0] = pools_[i];

        pthread_mutex_init(&mtxs_[i], nullptr);
    }
    pthread_mutex_init(&debugMtx_, nullptr);

    extendOffset_ = 1;
}

void MmapManager::ExtendBuffer(int core, size_t size)
{
    ASSERT(extendOffset_ < extendMax_, "extend max fault\n");

    auto devZero = -1;
    auto mmapSize = ALIGN(size, pageSize_);

    auto p = (void *)mmap(nullptr, mmapSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, devZero, 0);
    ASSERT(((void *)p != (void *)-1), "mmap result is -1: errno=%d\n", errno);
    // leak slightly, but it can be ignored
    pools_[core] = p;
    sizes_[core] = mmapSize;
    offsets_[core] = 0;
    debugSizes_[core] += mmapSize;
    // debugOffsets_[core] = 0;

    sizess_[extendMax_ * core + extendOffset_] = sizes_[core];
    poolss_[extendMax_ * core + extendOffset_] = pools_[core];
    extendOffset_++;
}

void MmapManager::FirstTouch(int core)
{
    ASSERT(((0 <= core) && (core < coreN_)) || core == mainThreadIndex_, "core = %d\n", core);
    if (mainThreadFlag) {
        core = mainThreadIndex_;
    }

    auto p = pools_[core];
    auto mmapSize = sizes_[core];
    for (auto of = 0u; of < mmapSize; of += pageSize_) {
        auto tmp = (char *)((uintptr_t)p + of);
        *tmp = 0;
    }
}

void *MmapManager::Malloc(int core, size_t size)
{
    ASSERT(((0 <= core) && (core < coreN_)) || core == mainThreadIndex_, "core = %d\n", core);
    if (mainThreadFlag) {
        core = mainThreadIndex_;
    }

    mtxlock l(mtxs_[core]);

    if (!firstTouchFlag_[core]) {
        FirstTouch(core);
        firstTouchFlag_[core] = true;
    }

    size_t restSize = sizes_[core] - offsets_[core];
    if (restSize < size) {
        if (forceMmapFlag_) {
            // extend pool
            auto allocSize = ((oneGB >> 4) > size) ? oneGB >> 4 : size;
            ExtendBuffer(core, allocSize);
            printf("\033[31m"); // red
            printf("extend mem : +%.3fGB ===> %.3fGB (core = %d)\n", (double)size / oneGB, (double)debugSizes_[core] / oneGB, core);
            printf("\033[00m"); // reset
            DebugPrintWithNoMalloc();
            // sizes_[core] maybe differ ater ExtendBuffer
            restSize = sizes_[core] - offsets_[core];
            if (restSize < size) {
                DebugPrintWithNoMalloc();
                ASSERT(false, "NO REST SIZE: core = %d, (req / rest size / max size) = (%ld/%ld/%ld)\n", core, size / oneMB, restSize / oneMB, debugSizes_[core] / oneMB);
                return nullptr;
            }
        }
    }
    auto p = (void *)((uintptr_t)pools_[core] + offsets_[core]);
    offsets_[core] += size;
    debugOffsets_[core] += size;

    return p;
}

void MmapManager::Free() {}
void MmapManager::Term()
{
#if 0
    for (int i = 0; i < threadN_; ++i) {
        for (auto j = 0; j < extendOffset_; ++j) {
            auto size = sizess_[extendMax_ * i + j];
            auto ptr   = poolss_[extendMax_ * i + j];
            munmap(ptr, size);
        }
    }
#endif
    fcmalloc::TypeAwareMemDeallocate(threadN_, debugOffsets_);
    fcmalloc::TypeAwareMemDeallocate(threadN_, debugSizes_);
    fcmalloc::TypeAwareMemDeallocate(threadN_ * extendMax_, poolss_);
    fcmalloc::TypeAwareMemDeallocate(threadN_ * extendMax_, sizess_);
    fcmalloc::TypeAwareMemDeallocate(threadN_, pools_);
    fcmalloc::TypeAwareMemDeallocate(threadN_, offsets_);
    fcmalloc::TypeAwareMemDeallocate(threadN_, sizes_);
    fcmalloc::TypeAwareMemDeallocate(threadN_, firstTouchFlag_);
    fcmalloc::TypeAwareMemDeallocate(threadN_, mtxs_);
}

void MmapManager::DebugPrintWithNoMalloc() const
{
    mtxlock l(debugMtx_);
    //  NOTE unit is MB
    for (auto i = 0; i < threadN_; ++i) {
        auto sizeMax  = (double)debugSizes_[i] / oneGB;
        auto sizeVal  = (double)debugOffsets_[i] / oneGB;
        auto ratio    = sizeVal / sizeMax;
        const auto charN = 48;
        auto tmp      = ratio;
        // reverse
        tmp = 1.0 - tmp;
        if (tmp < 0.1) {
            //  red
            printf("\033[31m");
        }
        else if (tmp < 0.25) {
            //  orange
            printf("\033[38;2;255;165;0m");
        }
        else if (tmp < 0.5) {
            //  yellow
            printf("\033[33m");
        }
        else if (tmp < 0.7) {
            //  green
            printf("\033[32m");
        }
        else if (tmp < 0.85) {
            //  blue
            printf("\033[34m");
        }
        else if (tmp < 0.999) {
            //  cyan
            printf("\033[36m");
        }
        else {
            //  magenta
            printf("\033[35m");
        }
        printf("%2d:[", i);
        for (auto i = 0; i < charN; ++i) {
            auto tmp = (double)(i + 1) / charN;
            if (tmp <= ratio || (i == charN && debugSizes_[i] == debugOffsets_[i])) {
                printf("=");
            }
            else {
                printf(" ");
            }
        }
        printf("]%.3f%%(%.4fGB/%.4fGB)\n", ratio, sizeVal, sizeMax);
        // reset
        printf("\033[00m");
    }
}

