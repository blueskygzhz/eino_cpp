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

#ifndef EINO_CPP_INTERNAL_SAFE_H_
#define EINO_CPP_INTERNAL_SAFE_H_

#include <string>
#include <stdexcept>
#include <sstream>

namespace eino {
namespace internal {
namespace safe {

// ============================================================================
// Safe Panic Error Handling - Aligns with eino internal/safe
// ============================================================================

// PanicError represents a wrapped panic with stack trace
// Aligns with eino internal/safe.panicErr (panic.go:16-20)
class PanicError : public std::runtime_error {
public:
    PanicError(const std::string& info, const std::string& stack)
        : std::runtime_error(FormatMessage(info, stack)),
          info_(info), stack_(stack) {}
    
    const std::string& GetInfo() const { return info_; }
    const std::string& GetStack() const { return stack_; }

private:
    static std::string FormatMessage(const std::string& info, const std::string& stack) {
        std::ostringstream oss;
        oss << "panic error: " << info << ", \nstack: " << stack;
        return oss.str();
    }

    std::string info_;
    std::string stack_;
};

// NewPanicErr creates a new panic error
// Aligns with eino internal/safe.NewPanicErr (panic.go:27-32)
inline PanicError NewPanicErr(const std::string& info, const std::string& stack) {
    return PanicError(info, stack);
}

// GetStackTrace returns a basic stack trace (simplified version)
// In production, use platform-specific stack trace utilities
inline std::string GetStackTrace() {
    // Simplified implementation
    // In real implementation, use backtrace() on Linux or similar
    return "[Stack trace not available in this build]";
}

} // namespace safe
} // namespace internal
} // namespace eino

#endif // EINO_CPP_INTERNAL_SAFE_H_
