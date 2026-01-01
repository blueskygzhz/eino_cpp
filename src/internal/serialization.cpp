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

#include "eino/internal/serialization.h"

namespace eino {
namespace internal {

// ============================================================================
// TypeRegistry Implementation
// ============================================================================

TypeRegistry::TypeRegistry() {
    // Auto-register basic types
    RegisterBasicTypes();
}

TypeRegistry& TypeRegistry::Instance() {
    static TypeRegistry instance;
    return instance;
}

// ============================================================================
// Basic Types Registration
// ============================================================================

void RegisterBasicTypes() {
    // Aligns with eino's init() in serialization.go that registers basic types
    auto& registry = TypeRegistry::Instance();
    
    // Integer types
    registry.Register<int>("_eino_int");
    registry.Register<int8_t>("_eino_int8");
    registry.Register<int16_t>("_eino_int16");
    registry.Register<int32_t>("_eino_int32");
    registry.Register<int64_t>("_eino_int64");
    
    // Unsigned integer types
    registry.Register<unsigned int>("_eino_uint");
    registry.Register<uint8_t>("_eino_uint8");
    registry.Register<uint16_t>("_eino_uint16");
    registry.Register<uint32_t>("_eino_uint32");
    registry.Register<uint64_t>("_eino_uint64");
    
    // Floating point types
    registry.Register<float>("_eino_float32");
    registry.Register<double>("_eino_float64");
    
    // Other basic types
    registry.Register<bool>("_eino_bool");
    registry.Register<std::string>("_eino_string");
    
    // Note: Complex types and uintptr_t omitted as they're less common in C++
}

}  // namespace internal
}  // namespace eino
