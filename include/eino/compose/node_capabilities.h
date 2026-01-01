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

#ifndef EINO_CPP_COMPOSE_NODE_CAPABILITIES_H_
#define EINO_CPP_COMPOSE_NODE_CAPABILITIES_H_

// Aligns with: eino/compose/graph_run.go (method selection logic)
// Purpose: Detect node capabilities and enable smart method selection

#include <memory>
#include <string>
#include <any>  // ⭐ Added: For std::any
#include "runnable.h"  // ⭐ Added: For StreamReader definition

namespace eino {
namespace compose {

// Forward declarations
template<typename I, typename O> class Runnable;
template<typename I, typename O> class ComposableRunnable;
template<typename T> class StreamReader;  // ⭐ Added: Forward declaration

// =============================================================================
// Node Capability Detection
// Aligns with: Go's runtime method checking via reflection
// =============================================================================

/**
 * @brief Detects what methods a Runnable implements
 * 
 * This is used to determine which execution method to call:
 * - Invoke: non-stream → non-stream
 * - Stream: non-stream → stream
 * - Collect: stream → non-stream
 * - Transform: stream → stream
 */
struct NodeCapabilities {
    bool has_invoke = false;
    bool has_stream = false;
    bool has_collect = false;
    bool has_transform = false;
    
    NodeCapabilities() = default;
    
    /**
     * @brief Detect capabilities by attempting to cast function pointers
     * 
     * In C++, we can't use reflection like Go, so we use template detection
     * and optional function pointer checking.
     */
    template<typename I, typename O>
    static NodeCapabilities Detect(std::shared_ptr<Runnable<I, O>> runnable);
    
    /**
     * @brief Check if node can handle stream input
     */
    bool CanHandleStreamInput() const {
        return has_collect || has_transform;
    }
    
    /**
     * @brief Check if node can produce stream output
     */
    bool CanProduceStreamOutput() const {
        return has_stream || has_transform;
    }
    
    /**
     * @brief Get preferred method for stream input
     * Priority: Transform > Collect > Invoke (with auto-collect)
     */
    std::string GetMethodForStreamInput() const {
        if (has_transform) return "Transform";
        if (has_collect) return "Collect";
        if (has_invoke) return "CollectThenInvoke";
        return "Unknown";
    }
    
    /**
     * @brief Get preferred method for regular input when downstream needs stream
     * Priority: Stream > Invoke (with auto-wrap)
     */
    std::string GetMethodForStreamOutput() const {
        if (has_stream) return "Stream";
        if (has_invoke) return "InvokeThenWrap";
        return "Unknown";
    }
};

// =============================================================================
// Runtime Type Detection Helper
// =============================================================================

/**
 * @brief Helper to detect if a type is StreamReader at runtime
 * 
 * Since C++ lacks Go's runtime reflection, we use type registration
 * and dynamic_cast checking.
 */
class TypeDetector {
public:
    /**
     * @brief Check if a std::any contains a StreamReader
     */
    template<typename T>
    static bool IsStreamReader(const std::any& value) {
        try {
            // Try to cast to StreamReader<T>
            auto ptr = std::any_cast<std::shared_ptr<StreamReader<T>>>(&value);
            return ptr != nullptr;
        } catch (...) {
            return false;
        }
    }
    
    /**
     * @brief Check if a void* points to a StreamReader (used in Graph execution)
     * 
     * ⭐ IMPLEMENTATION: Uses RTTI to check if the pointer is a StreamReader
     */
    static bool IsStreamReaderPtr(const std::shared_ptr<void>& ptr, 
                                   const std::type_info& element_type) {
        if (!ptr) {
            return false;
        }
        
        // Try to dynamic_cast to a known StreamReader type
        // This is a heuristic - in production, you'd need a type registry
        try {
            // Check common types
            if (std::dynamic_pointer_cast<StreamReader<std::string>>(ptr)) {
                return true;
            }
            if (std::dynamic_pointer_cast<StreamReader<int>>(ptr)) {
                return true;
            }
            // Add more types as needed
            return false;
        } catch (...) {
            return false;
        }
    }
    
    /**
     * @brief Extract StreamReader from type-erased pointer
     */
    template<typename T>
    static std::shared_ptr<StreamReader<T>> ExtractStreamReader(
        const std::shared_ptr<void>& ptr) {
        return std::static_pointer_cast<StreamReader<T>>(ptr);
    }
};

// =============================================================================
// Capability Detection Implementation (in node_capabilities.cpp)
// =============================================================================

// Detection is done by checking if methods throw "not implemented" errors
// or by registering capabilities explicitly in LambdaRunnable

template<typename I, typename O>
NodeCapabilities NodeCapabilities::Detect(std::shared_ptr<Runnable<I, O>> runnable) {
    NodeCapabilities caps;
    
    if (!runnable) {
        return caps;
    }
    
    auto ctx = Context::Background();
    
    // Test Invoke
    try {
        I dummy_input{};
        // Try to call Invoke with dummy input
        // If it doesn't throw "not implemented", it has Invoke
        // This is a heuristic - production code should use explicit registration
        caps.has_invoke = true;  // Assume all have Invoke as fallback
    } catch (...) {
        caps.has_invoke = false;
    }
    
    // For LambdaRunnable, we can check function pointers directly
    // This requires adding a method to LambdaRunnable:
    // bool HasInvokeFunc() const { return invoke_func_ != nullptr; }
    
    // For now, mark all methods as potentially available
    // Real detection requires either:
    // 1. Explicit capability registration in node construction
    // 2. Introspection via virtual "HasMethod" functions
    // 3. Try-catch with test inputs (expensive)
    
    caps.has_invoke = true;
    caps.has_stream = true;    // Optimistic - will fall back to Invoke
    caps.has_collect = true;   // Optimistic - will fall back to Invoke
    caps.has_transform = true; // Optimistic - will fall back to Invoke
    
    return caps;
}

// =============================================================================
// Capability Registration for LambdaRunnable (preferred approach)
// =============================================================================

/**
 * @brief Explicit capability registration
 * 
 * LambdaRunnable should expose which methods it actually implements,
 * avoiding expensive runtime detection.
 */
class ICapabilityProvider {
public:
    virtual ~ICapabilityProvider() = default;
    virtual NodeCapabilities GetCapabilities() const = 0;
};

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_NODE_CAPABILITIES_H_
