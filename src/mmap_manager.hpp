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

class MmapManager {
    public:
        void Init(int numCores, size_t pageSize, size_t mainSize, size_t subTotalSize);
        void *Malloc(int core, size_t size);
        void Free();
        void Term();

        void SetForceMmapFlag(bool forceMmapFlag) { forceMmapFlag_ = forceMmapFlag; }

        size_t GetPageSize() const { return pageSize_; }

    private:
        void ExtendBuffer(int core, size_t size);
        void FirstTouch(int core);

        void DebugPrintWithNoMalloc() const;

        int coreN_;
        int threadN_;
        int mainThreadIndex_;

        size_t pageSize_;

        pthread_mutex_t *mtxs_;
        bool* firstTouchFlag_;
        size_t* sizes_;
        size_t* offsets_;
        void** pools_;

        int extendMax_;
        int extendOffset_;
        size_t* sizess_;
        void** poolss_;

        bool forceMmapFlag_;

        mutable pthread_mutex_t debugMtx_;
        size_t* debugSizes_;
        size_t* debugOffsets_;
};
