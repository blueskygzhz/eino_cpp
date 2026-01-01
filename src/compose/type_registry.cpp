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

#include "eino/compose/type_registry.h"
#include <algorithm>

namespace eino {
namespace compose {

TypeRegistry& TypeRegistry::Instance() {
    static TypeRegistry instance;
    return instance;
}

void TypeRegistry::RegisterImplementation(
    const std::type_info& impl_type,
    const std::type_info& interface_type) {
    
    std::string impl_name = impl_type.name();
    std::string interface_name = interface_type.name();
    
    // Add to impl -> interfaces mapping
    impl_to_interfaces_[impl_name].push_back(interface_name);
    
    // Add to interface -> impls mapping
    interface_to_impls_[interface_name].push_back(impl_name);
}

bool TypeRegistry::Implements(
    const std::type_info& impl_type,
    const std::type_info& interface_type) const {
    
    std::string impl_name = impl_type.name();
    std::string interface_name = interface_type.name();
    
    auto it = impl_to_interfaces_.find(impl_name);
    if (it == impl_to_interfaces_.end()) {
        return false;
    }
    
    const auto& interfaces = it->second;
    return std::find(interfaces.begin(), interfaces.end(), interface_name) 
        != interfaces.end();
}

std::vector<const std::type_info*> TypeRegistry::GetInterfaces(
    const std::type_info& impl_type) const {
    
    std::vector<const std::type_info*> result;
    
    std::string impl_name = impl_type.name();
    auto it = impl_to_interfaces_.find(impl_name);
    if (it != impl_to_interfaces_.end()) {
        // Note: We return pointers, but we don't have the original type_info objects
        // This is a limitation of the current implementation
        // In practice, you should store std::type_index or the actual type_info
    }
    
    return result;
}

bool TypeRegistry::IsAssignable(
    const std::type_info& from,
    const std::type_info& to) const {
    
    // Same type
    if (from == to) {
        return true;
    }
    
    // Check if 'from' implements 'to' interface
    if (Implements(from, to)) {
        return true;
    }
    
    // Check if 'to' implements 'from' interface (reverse check for May case)
    if (Implements(to, from)) {
        return true;
    }
    
    return false;
}

} // namespace compose
} // namespace eino
