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

#ifndef EINO_CPP_INTERNAL_CHANNEL_H_
#define EINO_CPP_INTERNAL_CHANNEL_H_

#include <queue>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

namespace eino {
namespace internal {

// ============================================================================
// Unbounded Channel - Aligns with eino internal.UnboundedChan
// ============================================================================

// UnboundedChan represents a channel with unlimited capacity
// Aligns with eino internal.UnboundedChan (channel.go:19-25)
template<typename T>
class UnboundedChan {
public:
    UnboundedChan() : closed_(false) {}
    
    ~UnboundedChan() {
        Close();
    }

    // Send puts an item into the channel
    // Aligns with eino internal.UnboundedChan.Send (channel.go:33-42)
    void Send(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (closed_) {
            throw std::runtime_error("send on closed channel");
        }
        
        buffer_.push(value);
        not_empty_.notify_one();
    }

    // Receive gets an item from the channel (blocks if empty)
    // Aligns with eino internal.UnboundedChan.Receive (channel.go:44-60)
    bool Receive(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        while (buffer_.empty() && !closed_) {
            not_empty_.wait(lock);
        }
        
        if (buffer_.empty()) {
            // Channel is closed and empty
            return false;
        }
        
        value = buffer_.front();
        buffer_.pop();
        return true;
    }

    // Close marks the channel as closed
    // Aligns with eino internal.UnboundedChan.Close (channel.go:62-70)
    void Close() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (!closed_) {
            closed_ = true;
            not_empty_.notify_all();
        }
    }

    bool IsClosed() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return closed_;
    }

    size_t Size() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return buffer_.size();
    }

private:
    std::queue<T> buffer_;
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    bool closed_;
};

// NewUnboundedChan creates a new unbounded channel
// Aligns with eino internal.NewUnboundedChan (channel.go:27-31)
template<typename T>
std::shared_ptr<UnboundedChan<T>> NewUnboundedChan() {
    return std::make_shared<UnboundedChan<T>>();
}

} // namespace internal
} // namespace eino

#endif // EINO_CPP_INTERNAL_CHANNEL_H_
