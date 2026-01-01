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

#pragma once

#include <map>
#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

namespace eino {
namespace compose {

/**
 * TypeRegistry - Interface implementation registry system
 * Aligns with Go's reflect.Type.Implements() functionality
 * 
 * Solves the C++ limitation of runtime interface checking
 */
class TypeRegistry {
public:
    static TypeRegistry& Instance();
    
    /**
     * Register an implementation of an interface
     * @param impl_type The concrete implementation type
     * @param interface_type The interface (abstract base class) type
     */
    void RegisterImplementation(
        const std::type_info& impl_type,
        const std::type_info& interface_type);
    
    /**
     * Check if a type implements an interface
     * @param impl_type The concrete type to check
     * @param interface_type The interface type
     * @return true if impl_type implements interface_type
     */
    bool Implements(
        const std::type_info& impl_type,
        const std::type_info& interface_type) const;
    
    /**
     * Get all interfaces implemented by a type
     * @param impl_type The concrete type
     * @return List of interface types
     */
    std::vector<const std::type_info*> GetInterfaces(
        const std::type_info& impl_type) const;
    
    /**
     * Check if two types are compatible (same or inheritance)
     * @param from Source type
     * @param to Target type
     * @return true if from can be assigned to to
     */
    bool IsAssignable(
        const std::type_info& from,
        const std::type_info& to) const;

private:
    TypeRegistry() = default;
    
    // Map from implementation type to list of interfaces it implements
    std::map<std::string, std::vector<std::string>> impl_to_interfaces_;
    
    // Map from interface type to list of implementations
    std::map<std::string, std::vector<std::string>> interface_to_impls_;
};

/**
 * Auto-registration helper macro
 * Usage:
 *   EINO_REGISTER_IMPLEMENTATION(MyClass, IMyInterface);
 */
#define EINO_REGISTER_IMPLEMENTATION(ImplType, InterfaceType) \
    namespace { \
        struct TypeRegistrar_##ImplType##_##InterfaceType { \
            TypeRegistrar_##ImplType##_##InterfaceType() { \
                eino::compose::TypeRegistry::Instance() \
                    .RegisterImplementation(typeid(ImplType), typeid(InterfaceType)); \
            } \
        }; \
        static TypeRegistrar_##ImplType##_##InterfaceType \
            type_registrar_##ImplType##_##InterfaceType; \
    }

/**
 * Type info helper - stores runtime type information
 */
struct TypeInfo {
    const std::type_info* type = nullptr;
    std::string name;
    bool is_interface = false;
    
    TypeInfo() = default;
    
    explicit TypeInfo(const std::type_info& t)
        : type(&t), name(t.name()) {}
    
    bool operator==(const TypeInfo& other) const {
        return type && other.type && (*type == *other.type);
    }
    
    bool operator<(const TypeInfo& other) const {
        return type && other.type && type->before(*other.type);
    }
};

} // namespace compose
} // namespace eino
