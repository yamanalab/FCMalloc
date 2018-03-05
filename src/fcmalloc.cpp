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

#include "init_term.hpp"

#include "common.hpp"
#include "mmap_manager.hpp"
#include "common_memory_pool.hpp"
#include "global_memory_manager.hpp"
#include "local_memory_manager.hpp"
#include "memory_linked_list.hpp"
#include "memory_linked_list_manager.hpp"
#include "mem_allocate.hpp"
#include "memory_size_manager.hpp"

using namespace std;

void *malloc(size_t size);
void *realloc(void *ptr, size_t size) throw();
void *calloc(size_t nmemb, size_t size);
void free(void *p);

MmapManager mm;
thread_local bool mainThreadFlag = false;

namespace {
    GlobalMemoryManager g;
    thread_local LocalMemoryManager *lp = nullptr;
    CommonMemoryPool cmp;
    int numCores = 0;
    pthread_mutex_t dummy_mtx = PTHREAD_MUTEX_INITIALIZER;

    MemorySizeManager& msm()
    {
        static MemorySizeManager msm;
        return msm;
    }
}

void mainInit()
{
    mainThreadFlag = true;

    numCores = sysconf(_SC_NPROCESSORS_ONLN);
    auto pageSize = sysconf(_SC_PAGESIZE);

    const size_t unit1GB       = 1024 * 1024 * 1024;
    const size_t unit1MB       = 1024 * 1024;
    auto forceExtendMemFlag = true;
    auto forceExtendMemStr = getenv("FCM_FORCE_EXTEND_MEM_FLAG");
    if(forceExtendMemStr) {
        forceExtendMemFlag = (atoi(forceExtendMemStr) > 0);
    }
    auto mainMemoryMax    = 32u;
    auto mainMemoryMaxStr = getenv("FCM_MAIN_MEM_MAX");
    if(mainMemoryMaxStr) {
        mainMemoryMax = atoll(mainMemoryMaxStr);
    }
    auto subMemoryMax     = 4u * numCores;
    auto subMemoryMaxStr = getenv("FCM_SUB_MEM_MAX");
    if(subMemoryMaxStr) {
        subMemoryMax = atoll(subMemoryMaxStr);
    }
    mm.Init(numCores, pageSize, mainMemoryMax * unit1MB, subMemoryMax * unit1MB);
    mm.SetForceMmapFlag(forceExtendMemFlag);
    cmp.Init(numCores, msm());
    g.Init(numCores, cmp, msm());
}
void mainTerm()
{
    mm.Term();
}

void threadInit()
{
    int core;
    core = sched_getcpu();
    lp = g.AllocLocalMemoryManager(core);
    ASSERT(lp != nullptr, "lp is null\n");
}
void threadTerm()
{
    if (lp != nullptr) {
        g.FreeLocalMemoryManager(lp);
        lp = nullptr;
    }
}

void *malloc(size_t size)
{
    init_();

    if (size == 0) {
        return nullptr;
    }

    ASSERT(lp != nullptr, "lp is null\n");
    void *ptr = lp->Malloc(size);
    return ptr;
}
void free(void *ptr)
{
    init_();

    if (ptr == nullptr) {
        return;
    }
    ASSERT(ALIGN_CHECK(ptr, 16), "free addr. align. error %ld\n", ALIGN_REMAIN(ptr, 16));

    if (lp == nullptr) {
        g.Free(ptr);
        return;
    }
    ASSERT(lp != nullptr, "lp is null\n");
    lp->Free(ptr);

    {
        static thread_local int cntsPerSize[MemorySizeManager::Size];
        size_t size = MemUtil::PtrToSize(ptr);
        int index = logarithm2(size);
        int freeCntInterval = msm().GetMemorySize(index);
        ASSERT(freeCntInterval > 0, "Please cahnge n per size! size = %ld, 2^x(x=%d)\n", size, index);
        ++cntsPerSize[index];
        if (mainThreadFlag && cntsPerSize[index] > freeCntInterval) {
            memset(cntsPerSize, 0, MemorySizeManager::Size * sizeof(int));
            lp->AllFreeToCommonMemoryPool();
        }
    }
}

void *calloc(size_t nmemb, size_t size)
{
    init_();

    if (nmemb == 0 || size == 0) {
        return nullptr;
    }

    const size_t total_size = nmemb * size;
    //Debug("[calloc pre]: nmemb = %ld, size = %ld, total_size = %ld\n", nmemb, size, total_size);
    void *ptr = malloc(total_size);
    if (ptr != nullptr) {
        memset(ptr, 0, total_size);
    }
    //Debug("[calloc]: ptr = %p, nmemb = %ld, size = %ld, total_size = %ld\n", ptr, nmemb, size, total_size);
    ASSERT(ALIGN_CHECK(ptr, 16), "calloc addr. align. error %ld\n", ALIGN_REMAIN(ptr, 16));
    return ptr;
}

void *realloc(void *ptr, size_t size) throw()
{
    init_();

    if (ptr == nullptr) {
        return malloc(size);
    }
    ASSERT(ALIGN_CHECK(ptr, 16), "realloc addr. align. error %ld\n", ALIGN_REMAIN(ptr, 16));

    ASSERT(lp != nullptr, "lp is null\n");
    void *newPtr = lp->Realloc(ptr, size);
    return newPtr;
}