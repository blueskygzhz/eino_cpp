/*
 * Copyright 2024 CloudWeGo Authors
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

#ifndef EINO_CPP_ADK_ASYNC_ITERATOR_H_
#define EINO_CPP_ADK_ASYNC_ITERATOR_H_

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace eino {
namespace adk {

// Forward declarations
template <typename T> class AsyncIterator;

// AsyncGenerator is used to send values to AsyncIterator
template <typename T>
class AsyncGenerator {
public:
    void Send(T value) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            queue_.push(value);
        }
        cv_.notify_one();
    }

    void Close() {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            closed_ = true;
        }
        cv_.notify_one();
    }

    friend class AsyncIterator<T>;

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool closed_ = false;
};

// AsyncIterator is used to receive values from AsyncGenerator
template <typename T>
class AsyncIterator {
public:
    explicit AsyncIterator(std::shared_ptr<AsyncGenerator<T>> generator)
        : generator_(generator) {}

    bool Next(T& value) {
        std::unique_lock<std::mutex> lock(generator_->mutex_);

        while (generator_->queue_.empty() && !generator_->closed_) {
            generator_->cv_.wait(lock);
        }

        if (generator_->queue_.empty()) {
            return false;
        }

        value = generator_->queue_.front();
        generator_->queue_.pop();
        return true;
    }

    bool HasNext() {
        std::unique_lock<std::mutex> lock(generator_->mutex_);
        return !generator_->queue_.empty() || !generator_->closed_;
    }

private:
    std::shared_ptr<AsyncGenerator<T>> generator_;
};

// Create a pair of AsyncIterator and AsyncGenerator
template <typename T>
inline std::pair<std::shared_ptr<AsyncIterator<T>>, std::shared_ptr<AsyncGenerator<T>>>
NewAsyncIteratorPair() {
    auto generator = std::make_shared<AsyncGenerator<T>>();
    auto iterator = std::make_shared<AsyncIterator<T>>(generator);
    return {iterator, generator};
}

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_ASYNC_ITERATOR_H_
