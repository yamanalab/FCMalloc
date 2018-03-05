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

#include "memory_linked_list_manager.hpp"

void MemoryLinkedListManager::append(int index, MemoryLinkedList *thead, MemoryLinkedList *tlast)
{
    ASSERT(thead != nullptr, "thead pointer must not be nullptr\n");
    ASSERT(tlast != nullptr, "tlast pointer must not be nullptr\n");
    ASSERT(tlast->GetNext() == nullptr, "last next pointer must be nullptr\n");

    MemoryLinkedList *&head = heads_[index];
    MemoryLinkedList *&last = lasts_[index];

    // if lengthAssertFlag is true, long length search takes a lot of time when freeing memory
    auto lengthAssertFlag = false;
    auto preLength = 0u;
    if (lengthAssertFlag && head != nullptr) {
        preLength = head->GetLength();
    }

    if (head == nullptr) {
        // length = 0 -> init
        head = thead;
        last = tlast;
    }
    else {
        // append
        last->SetNext(thead);
        last = tlast;
    }

    if (lengthAssertFlag) {
        auto postLength   = head->GetLength();
        auto appendLength = thead->GetLength();
        ASSERT(appendLength > 0, "append length must be more than 0\n");
        ASSERT(preLength + appendLength == postLength, "if single thread must same pre = %ld + %ld, post = %ld\n", preLength, appendLength, postLength);
    }

    ASSERT(last->GetNext() == nullptr, "last next pointer must be nullptr\n");
}

void MemoryLinkedListManager::Allocate(int core, size_t size, size_t n)
{
    auto index = logarithm2(size);
    auto ret = allocateMemoryLinkedList(coreN_, core, size, n);
    append(index, ret.head, ret.last);
}

MemoryLinkedList *MemoryLinkedListManager::popN(int index, int n)
{
    MemoryLinkedList *&head = heads_[index];
    MemoryLinkedList *&last = lasts_[index];

    if (head == nullptr || last == nullptr) {
        errno = ENOMEM;
        return nullptr;
    }
    head->Assert();

    auto ret = head;

    auto newHead = head->GetNext();
    auto tmp = newHead;
    for (auto i = 1; i < n && tmp->GetNext() != nullptr; ++i) {
        tmp = tmp->GetNext();
    }
    auto newLast = tmp;
    head = newLast->GetNext();
    newLast->SetNext(nullptr);
    if (head != nullptr) {
        head->Assert();
    }
    else {
        last = nullptr;
    }
    return ret;
}

MemoryLinkedList *MemoryLinkedListManager::pop(int index)
{
    MemoryLinkedList *&head = heads_[index];
    MemoryLinkedList *&last = lasts_[index];

    if (head == nullptr || last == nullptr) {
        errno = ENOMEM;
        return nullptr;
    }
    head->Assert();

    auto ret = head;

    auto newHead = head->GetNext();
    head->SetNext(nullptr);
    if (newHead != nullptr) {
        newHead->Assert();
    }
    else {
        last = nullptr;
    }
    head = newHead;
    return ret;
}

void *MemoryLinkedListManager::Malloc(size_t size)
{
    int index = logarithm2(size);
    MemoryLinkedList *&head = heads_[index];
    MemoryLinkedList *&last = lasts_[index];

    if (head == nullptr || last == nullptr) {
        errno = ENOMEM;
        return nullptr;
    }
    head->Assert();

    auto ptr = head->GetBodyAddr();
    ASSERT((ptr != nullptr), "malloced addr must not be null\n");
    ASSERT((size <= head->GetBodySize()), "size = %ld, malloced_size = %ld\n", size, head->GetBodySize());

    auto newHead = pop(index);
    return ptr;
}

void MemoryLinkedListManager::push(int index, MemoryLinkedList *next)
{
    append(index, next, next);
}

void MemoryLinkedListManager::Free(void *ptr)
{
    if (ptr == nullptr) {
        return;
    }

    auto m = (MemoryLinkedList *)((uintptr_t)ptr - sizeof(MemoryLinkedList));
    m->Assert();
    ASSERT((m->GetNext() == nullptr), "free next ptr must be null\n");

    auto size = m->GetBodySize();
    auto index = logarithm2(size);

    push(index, m);
    return;
}

void MemoryLinkedListManager::Join(MemoryLinkedListManager *lm)
{
    for (auto i = 0; i < MemorySizeManager::Size; ++i) {
        if (lm->heads_[i] != nullptr) {
            append(i, lm->heads_[i], lm->lasts_[i]);
            lm->heads_[i] = nullptr;
            lm->lasts_[i] = nullptr;
        }
    }
}
