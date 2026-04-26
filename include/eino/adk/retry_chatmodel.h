/*
 * Copyright 2025 CloudWeGo Authors
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

#ifndef EINO_CPP_ADK_RETRY_CHATMODEL_H_
#define EINO_CPP_ADK_RETRY_CHATMODEL_H_

// Retry wrapper for ChatModel calls.
// Aligned with Go: adk/retry_chatmodel.go
//
// Provides automatic retry logic with configurable:
// - Maximum retry attempts
// - Retryable error detection
// - Backoff strategy (default: exponential with jitter)

#include <chrono>
#include <cmath>
#include <functional>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include "eino/adk/handler.h"
#include "eino/internal/core/address.h"

namespace eino {
namespace adk {

// RetryExhaustedError is returned when all retry attempts have been exhausted.
// Aligned with Go: adk.RetryExhaustedError
class RetryExhaustedError : public std::runtime_error {
public:
    RetryExhaustedError(const std::string& last_err, int total_retries)
        : std::runtime_error(
              last_err.empty() ? "exceeds max retries"
                               : "exceeds max retries: last error: " + last_err),
          last_err_(last_err),
          total_retries_(total_retries) {}

    const std::string& GetLastError() const { return last_err_; }
    int GetTotalRetries() const { return total_retries_; }

private:
    std::string last_err_;
    int total_retries_;
};

// WillRetryError is emitted when a retryable error occurs and a retry will be attempted.
// Aligned with Go: adk.WillRetryError
class WillRetryError : public std::runtime_error {
public:
    WillRetryError(const std::string& err_str, int retry_attempt)
        : std::runtime_error(err_str),
          err_str_(err_str),
          retry_attempt_(retry_attempt) {}

    const std::string& GetErrStr() const { return err_str_; }
    int GetRetryAttempt() const { return retry_attempt_; }

private:
    std::string err_str_;
    int retry_attempt_;
};

// BackoffFunc calculates the delay before the next retry attempt.
// Aligned with Go: ModelRetryConfig.BackoffFunc
using BackoffFunc = std::function<std::chrono::milliseconds(int attempt)>;

// IsRetryableFunc determines whether an error should trigger a retry.
// Aligned with Go: ModelRetryConfig.IsRetryAble
using IsRetryableFunc = std::function<bool(const std::string& err)>;

// Default backoff: exponential with jitter, base 100ms, max 10s.
// Aligned with Go: adk.defaultBackoff()
inline std::chrono::milliseconds DefaultBackoff(int attempt) {
    const int64_t base_delay_ms = 100;
    const int64_t max_delay_ms = 10000;

    if (attempt <= 0) {
        return std::chrono::milliseconds(base_delay_ms);
    }

    if (attempt > 7) {
        static std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<int64_t> dist(0, max_delay_ms / 2);
        return std::chrono::milliseconds(max_delay_ms + dist(gen));
    }

    int64_t delay_ms = base_delay_ms * (1LL << (attempt - 1));
    if (delay_ms > max_delay_ms) {
        delay_ms = max_delay_ms;
    }

    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int64_t> dist(0, delay_ms / 2);
    return std::chrono::milliseconds(delay_ms + dist(gen));
}

// Default retryable check: all errors are retryable.
// Aligned with Go: adk.defaultIsRetryAble()
inline bool DefaultIsRetryable(const std::string& /*err*/) {
    return true;
}

// Full ModelRetryConfig with all fields.
// Aligned with Go: adk.ModelRetryConfig (extended version)
struct FullModelRetryConfig {
    // MaxRetries specifies the maximum number of retry attempts.
    // A value of 0 means no retries will be attempted.
    int max_retries = 0;

    // IsRetryable determines whether an error should trigger a retry.
    // If null, all errors are considered retryable.
    IsRetryableFunc is_retryable;

    // BackoffFunc calculates the delay before the next retry attempt.
    // If null, default exponential backoff with jitter is used.
    BackoffFunc backoff_func;

    FullModelRetryConfig() = default;
    explicit FullModelRetryConfig(int max_retries)
        : max_retries(max_retries) {}
};

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_RETRY_CHATMODEL_H_
