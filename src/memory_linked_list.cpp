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

#include "memory_linked_list.hpp"
#include "mmap_manager.hpp"

extern MmapManager mm;

void MemoryLinkedList::Init(size_t numCores)
{
    dummy_space_ = SIGNATURE;
    coreN_       = numCores;
    core_        = -1;
    size_        = 0;
    next_        = nullptr;
    bodyAddr_    = nullptr;
}

bool MemoryLinkedList::CheckSignature() const
{
    //Debug("[mem pre check SIG]\n");
    return (dummy_space_ == SIGNATURE);
}

void MemoryLinkedList::Assert() const
{
    //Debug("[mem pre assert]\n");
    //Debug("[mem assert] ptr = %p, SIG = %x, size = %u, core = %u\n", this, dummy_space_, size_, core_);

    ASSERT((dummy_space_ == SIGNATURE), "sig check fault: SIG = %u\n", dummy_space_);
    ASSERT(((0 <= core_) && (core_ < coreN_)), "core = %u\n", core_);
    ASSERT((size_ > 0), "size is 0\n");
    ASSERT((bodyAddr_ != nullptr), "pointer is null\n");
}

MemoryLinkedListResult allocateMemoryLinkedList(size_t numCores, size_t core, size_t size, size_t n)
{
    ASSERT(((0 <= core) && (core < numCores)), "core = %u\n", core);
    ASSERT((size > 0), "size is 0\n");
    ASSERT((n > 0), "n is 0\n");

    size = roundup_powerof2(size);
    //  size==1, 2, 4 -> 8B align.
    if (size < 8) size = 8;
    ASSERT(ALIGN_CHECK(size, 8), "size align.\n");

    ASSERT(ALIGN_CHECK(sizeof(MemoryLinkedList), 16), "mem linked list size align.\n");

    //  check overflow
    OVERFLOW_ADD_ASSERT(size, ALIGN(sizeof(MemoryLinkedList), 16));
    auto totalSize = size + ALIGN(sizeof(MemoryLinkedList), 16);
    //  size==1, 2, 4, 8 -> 16B align.
    totalSize = ALIGN(totalSize, 16);

    auto pageSize = mm.GetPageSize();
    auto mmapSize = ALIGN(totalSize * n, pageSize);

    ASSERT(ALIGN_CHECK(mmapSize, pageSize), "mmap pagesize falt.\n");

    auto p = mm.Malloc(core, mmapSize);

    auto bodySize = size;

    auto buffer = p;
    ASSERT(ALIGN_CHECK(buffer, 16), "buffer = %p, buffer %% 16 = %ld\n", buffer, (uintptr_t)buffer % 16);

    MemoryLinkedList* head = nullptr;
    MemoryLinkedList* pre  = nullptr;
    for (auto i = 0u; i < n; ++i) {
        auto m = (MemoryLinkedList *)(buffer);
        m->Init(numCores);

        m->SetCore(core);
        m->SetSize(bodySize);
        m->SetBodyAddr((void*)((uintptr_t)buffer + m->GetHeaderSize()));
        if (head == nullptr) {
            head = m;
        }
        if (pre != nullptr) {
            pre->SetNext(m);
        }
        buffer = (void*)((uintptr_t)buffer + totalSize);
        pre = m;
        ASSERT(ALIGN_CHECK(m->GetBodyAddr(), 16), "body align check falt.\n");
    }

    auto last = pre;
    return MemoryLinkedListResult{ p, head, last };
}
