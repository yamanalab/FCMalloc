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

void mainThreadLocalDummyFunc_();
void threadLocalDummyFunc_();

void init_()
{
    mainThreadLocalDummyFunc_();
    threadLocalDummyFunc_();
}

//  main thread
namespace {
    typedef struct main_thread_caller {
        main_thread_caller() { mainInit(); }
        ~main_thread_caller() { mainTerm(); }
    } main_thread_caller_t;
}

void mainThreadLocalDummyFunc_() { static main_thread_caller_t _tmp; }

//  thread local
namespace {
    typedef struct thread_local_caller {
        thread_local_caller() { threadInit(); }
        ~thread_local_caller() { threadTerm(); }
    } thread_local_caller_t;
}

void threadLocalDummyFunc_() { thread_local static thread_local_caller_t _tmp; }
