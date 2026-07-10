/**
    Copyright [stag-project] [stag-project]

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
#ifndef STAGDEER_THREAD
#define STAGDEER_THREAD

#define CLIENT_THREAD_INIT(__M_thread_number__)\
stagdeer::THREAD& thread = stagdeer::THREAD::getInstance();\
    thread.createThreadManager(__M_thread_number__)
    #define CLIENT_THREAD_WITA()\
        stagdeer::THREAD& __M_wita_thread__ = stagdeer::THREAD::getInstance();\
            __M_wita_thread__.waitAllthread()

#include "threadTask.hpp"
#include <atomic>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <thread>
#include <chrono>

namespace stagdeer {
    class THREAD {
        public:
            THREAD() = default;
            ~THREAD() {
                if (M_threadmanager) {
                    M_threadmanager->waitAllthread();
                }
            }

            static THREAD& getInstance() {
                static THREAD M_instance_this;
                return M_instance_this;
            }

            static void waitThread() {
                stagdeer::THREAD::getInstance().getThreadManager().waitAllthread();
            }

            void createThreadManager(size_t threadNumber) {
                if (M_is_init_threadmnager.load()) {
                    return;
                }
                M_threadmanager = std::make_unique<stagdeer::ThreadManager>(threadNumber);
                M_is_init_threadmnager.store(true);
                return;
            }

            ThreadManager& getThreadManager() {
                if (!M_is_init_threadmnager.load()) {
                    throw std::runtime_error("threadManager not initialized");
                }
                return *M_threadmanager;
            }

            THREAD(THREAD&& M_other_Thread) = delete;
            THREAD&& operator=(THREAD&& M_other_Operator) = delete;
            THREAD(const THREAD&) = delete;
            THREAD& operator=(const THREAD&) = delete;
            private:
                std::unique_ptr<stagdeer::ThreadManager> M_threadmanager;
                std::atomic<bool> M_is_init_threadmnager{false};
    };
}

#endif