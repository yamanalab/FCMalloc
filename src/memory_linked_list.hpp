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

#include <cerrno>

class MemoryLinkedList {
    public:
        size_t GetLength() const
        {
            if (IsLast()) {
                return 1;
            }
            auto cnt = 1;
            for (auto ptr = this; !ptr->IsLast(); ptr = ptr->GetNext()) {
                ASSERT(ptr != nullptr, "must not be nullptr\n");
                ++cnt;
            }
            return cnt;
        }

        void Init(size_t numCores);
        void SetCore(int core)
        {
            ASSERT(((0 <= core) && (core < coreN_)), "core = %d\n", core);
            core_ = core;
        }
        void SetSize(size_t size) { size_ = size; }
        void SetNext(MemoryLinkedList *next) { next_ = next; }
        void SetBodyAddr(void *bodyAddr) { bodyAddr_ = bodyAddr; }

        bool CheckSignature() const;

        int GetCore() const { return core_; }
        MemoryLinkedList *GetNext() const { return next_; }
        void *GetBodyAddr() const { return bodyAddr_; }

        void Assert() const;
        bool IsLast() const { return (next_ == nullptr); }
        size_t GetHeaderSize() const { return ALIGN(sizeof(MemoryLinkedList), 16); }
        size_t GetBodySize() const { return size_; }
        size_t GetTotalSize() const
        {
            //  check overflow
            OVERFLOW_ADD_ASSERT(sizeof(MemoryLinkedList), size_);
            return sizeof(MemoryLinkedList) + size_;
        }

    private:
        // NOTE sizeof()で取得した際に16B ALIGNにしたいので、サイズに注意
        size_t dummy_space_;     // Verification Code Space
        size_t core_;            // allocated core id
        size_t coreN_;
        size_t size_;            // max raw data size
        MemoryLinkedList *next_; //  linked list pointer
        void *bodyAddr_;         // raw data
};

struct MemoryLinkedListResult {
    void *mmapAddr;
    MemoryLinkedList *head;
    MemoryLinkedList *last;
};

MemoryLinkedListResult allocateMemoryLinkedList(size_t numCores, size_t core, size_t size, size_t n);
