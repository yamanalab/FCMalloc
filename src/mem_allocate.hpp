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

namespace fcmalloc {

    void *MemAllocate(const size_t size);

    template<typename T>
        size_t TypeAwareMemAllocate(size_t numElements, T** p) {
            auto size = numElements * sizeof(T);
            *p = (T*)MemAllocate(size);
            return size;
        }

    void MemDeallocate(size_t size, void* p);

    template<typename T>
        void TypeAwareMemDeallocate(size_t numElements, T* p) {
            MemDeallocate(numElements * sizeof(T), (void *)p);
        }

} // namespace fcmalloc

