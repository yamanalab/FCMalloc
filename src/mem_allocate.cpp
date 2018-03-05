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

#include <errno.h>
#include <sys/mman.h>

#include "common.hpp"
#include "mem_allocate.hpp"

namespace fcmalloc {

    void *MemAllocate(const size_t size) {
        if (size == 0) { return nullptr; }
        void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        ASSERT(p != MAP_FAILED, "mmap failed: errno=%d\n", errno);
        return p;
    }

    void MemDeallocate(const size_t size, void* p) {
        if (size == 0 || p == nullptr) { return; }
        munmap(p, size);
    }

} // namespace fcmalloc
