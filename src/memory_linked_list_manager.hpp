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

#include "memory_size_manager.hpp"
#include "memory_linked_list.hpp"

class MemoryLinkedListManager {
    public:
        void Init(int numCores)
        {
            coreN_ = numCores;
            for (auto i = 0; i < MemorySizeManager::Size; ++i) {
                heads_[i] = nullptr;
                lasts_[i] = nullptr;
            }
        }
        void append(int index, MemoryLinkedList *head, MemoryLinkedList *last);
        MemoryLinkedList *pop(int index);
        MemoryLinkedList *popN(int index, int n);
        void push(int index, MemoryLinkedList *next);

        void Allocate(int core, size_t size, size_t n);
        void *Malloc(size_t size);

        //  index == log2(size)
        void Swap(MemoryLinkedListManager *dst, int index)
        {
            ASSERT(0 <= index && index < MemorySizeManager::Size, "log2\n");
            ASSERT(dst != nullptr, "dst is nullptr\n");
            {
                auto tmp = heads_[index];
                heads_[index] = dst->heads_[index];
                dst->heads_[index] = tmp;
            }
            {
                auto tmp = lasts_[index];
                lasts_[index] = dst->lasts_[index];
                dst->lasts_[index] = tmp;
            }
        }

        void Free(void *ptr);

        size_t GetAllFreeLength() const
        {
            auto cnt = 0u;
            for (auto i = 0; i < MemorySizeManager::Size; ++i) {
                if (heads_[i] != nullptr) {
                    cnt += heads_[i]->GetLength();
                }
            }
            return cnt;
        }
        void Join(MemoryLinkedListManager *lm);

    private:
        int coreN_;

        MemoryLinkedList *heads_[MemorySizeManager::Size];
        MemoryLinkedList *lasts_[MemorySizeManager::Size];
};
