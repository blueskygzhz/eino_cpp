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

#ifndef EINO_CPP_INTERNAL_SERIALIZATION_H_
#define EINO_CPP_INTERNAL_SERIALIZATION_H_

#include <nlohmann/json.hpp>
#include <string>
#include <map>
#include <typeindex>
#include <typeinfo>
#include <functional>
#include <memory>
#include <stdexcept>

namespace eino {
namespace internal {

// ============================================================================
// TypeRegistry - Aligns with eino internal/serialization type registration
// ============================================================================

/**
 * TypeRegistry manages type registration for serialization/deserialization
 * Aligns with eino internal/serialization.go:
 *   - var m = map[string]reflect.Type{}
 *   - var rm = map[reflect.Type]string{}
 *   - func GenericRegister[T any](key string) error
 */
class TypeRegistry {
public:
    static TypeRegistry& Instance();
    
    /**
     * Register a type with a unique key
     * Aligns with eino GenericRegister[T](key string)
     * 
     * @tparam T Type to register
     * @param type_name Unique identifier for the type
     * @return true if registered successfully, false if already registered
     */
    template<typename T>
    bool Register(const std::string& type_name) {
        std::type_index type_idx = typeid(T);
        
        // Check if type name already used
        if (name_to_type_.count(type_name)) {
            throw std::runtime_error(
                "Type name '" + type_name + "' already registered");
        }
        
        // Check if type already registered
        if (type_to_name_.count(type_idx)) {
            throw std::runtime_error(
                "Type '" + std::string(typeid(T).name()) + 
                "' already registered as '" + type_to_name_[type_idx] + "'");
        }
        
        // Register type info
        name_to_type_[type_name] = type_idx;
        type_to_name_[type_idx] = type_name;
        
        // Register serialization function
        serializers_[type_idx] = [](const void* obj) -> nlohmann::json {
            return SerializeValue(*static_cast<const T*>(obj));
        };
        
        // Register deserialization function
        deserializers_[type_idx] = [](const nlohmann::json& j) -> std::shared_ptr<void> {
            return std::make_shared<T>(DeserializeValue<T>(j));
        };
        
        return true;
    }
    
    /**
     * Get type name for a type
     * @tparam T Type to query
     * @return Type name string, or empty if not registered
     */
    template<typename T>
    std::string GetTypeName() const {
        std::type_index type_idx = typeid(T);
        auto it = type_to_name_.find(type_idx);
        if (it != type_to_name_.end()) {
            return it->second;
        }
        return "";
    }
    
    /**
     * Get type index for a type name
     * @param type_name Type name
     * @return Type index, or nullptr if not found
     */
    std::type_index GetTypeIndex(const std::string& type_name) const {
        auto it = name_to_type_.find(type_name);
        if (it != name_to_type_.end()) {
            return it->second;
        }
        throw std::runtime_error("Type name '" + type_name + "' not registered");
    }
    
    /**
     * Serialize an object of registered type
     * @param type_idx Type index
     * @param obj Pointer to object
     * @return Serialized JSON
     */
    nlohmann::json Serialize(std::type_index type_idx, const void* obj) const {
        auto it = serializers_.find(type_idx);
        if (it == serializers_.end()) {
            throw std::runtime_error("Type not registered for serialization");
        }
        return it->second(obj);
    }
    
    /**
     * Deserialize an object of registered type
     * @param type_idx Type index
     * @param j JSON to deserialize
     * @return Shared pointer to deserialized object
     */
    std::shared_ptr<void> Deserialize(std::type_index type_idx, const nlohmann::json& j) const {
        auto it = deserializers_.find(type_idx);
        if (it == deserializers_.end()) {
            throw std::runtime_error("Type not registered for deserialization");
        }
        return it->second(j);
    }
    
private:
    TypeRegistry();
    ~TypeRegistry() = default;
    TypeRegistry(const TypeRegistry&) = delete;
    TypeRegistry& operator=(const TypeRegistry&) = delete;
    
    // Type name -> Type index
    std::map<std::string, std::type_index> name_to_type_;
    
    // Type index -> Type name  
    std::map<std::type_index, std::string> type_to_name_;
    
    // Type index -> Serialization function
    std::map<std::type_index, std::function<nlohmann::json(const void*)>> serializers_;
    
    // Type index -> Deserialization function
    std::map<std::type_index, std::function<std::shared_ptr<void>(const nlohmann::json&)>> deserializers_;
};

// ============================================================================
// Serialization Functions - Aligned with eino InternalSerializer
// ============================================================================

/**
 * SerializeValue - Default serialization for types supporting to_json
 * Specialized templates should be provided for complex types
 */
template<typename T>
nlohmann::json SerializeValue(const T& value) {
    // Default: use nlohmann::json's automatic conversion
    return nlohmann::json(value);
}

/**
 * DeserializeValue - Default deserialization for types supporting from_json
 * Specialized templates should be provided for complex types
 */
template<typename T>
T DeserializeValue(const nlohmann::json& j) {
    // Default: use nlohmann::json's automatic conversion
    return j.get<T>();
}

/**
 * Serialize - Top-level serialization function
 * Aligns with eino InternalSerializer.Marshal()
 * 
 * Serializes any registered type to JSON format:
 * {
 *   "__type__": "TypeName",
 *   "__value__": <serialized value>
 * }
 * 
 * @tparam T Type to serialize (must be registered)
 * @param obj Object to serialize
 * @return JSON representation
 */
template<typename T>
nlohmann::json Serialize(const T& obj) {
    nlohmann::json result;
    
    std::string type_name = TypeRegistry::Instance().GetTypeName<T>();
    if (type_name.empty()) {
        throw std::runtime_error(
            "Type '" + std::string(typeid(T).name()) + "' not registered");
    }
    
    result["__type__"] = type_name;
    result["__value__"] = SerializeValue(obj);
    
    return result;
}

/**
 * Deserialize - Top-level deserialization function
 * Aligns with eino InternalSerializer.Unmarshal()
 * 
 * @tparam T Type to deserialize to
 * @param j JSON containing {"__type__": ..., "__value__": ...}
 * @return Deserialized object
 */
template<typename T>
T Deserialize(const nlohmann::json& j) {
    if (!j.contains("__type__") || !j.contains("__value__")) {
        throw std::runtime_error(
            "Invalid serialized data: missing __type__ or __value__");
    }
    
    std::string type_name = j["__type__"].get<std::string>();
    std::string expected_name = TypeRegistry::Instance().GetTypeName<T>();
    
    if (type_name != expected_name) {
        throw std::runtime_error(
            "Type mismatch: expected '" + expected_name + 
            "' but got '" + type_name + "'");
    }
    
    return DeserializeValue<T>(j["__value__"]);
}

/**
 * SerializeToString - Serialize object to JSON string
 * @tparam T Type to serialize
 * @param obj Object to serialize
 * @return JSON string
 */
template<typename T>
std::string SerializeToString(const T& obj) {
    return Serialize(obj).dump();
}

/**
 * DeserializeFromString - Deserialize object from JSON string
 * @tparam T Type to deserialize to
 * @param json_str JSON string
 * @return Deserialized object
 */
template<typename T>
T DeserializeFromString(const std::string& json_str) {
    nlohmann::json j = nlohmann::json::parse(json_str);
    return Deserialize<T>(j);
}

// ============================================================================
// Convenience Registration Functions
// ============================================================================

/**
 * RegisterBasicTypes - Register all basic C++ types
 * Aligns with eino's init() function that registers int, float, string, etc.
 * Called automatically on library load
 */
void RegisterBasicTypes();

}  // namespace internal
}  // namespace eino

#endif  // EINO_CPP_INTERNAL_SERIALIZATION_H_
