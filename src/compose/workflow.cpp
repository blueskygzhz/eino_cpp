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

#include "eino/compose/workflow.h"
#include <sstream>
#include <vector>
#include <map>
#include <string>

namespace eino {
namespace compose {

// WorkflowNode implementation
WorkflowNode::WorkflowNode(const std::string& key)
    : key_(key) {}

WorkflowNode::~WorkflowNode() = default;

WorkflowNode& WorkflowNode::AddInput(
    const std::string& from_node_key,
    const std::vector<FieldMapping>& inputs) {
    return AddInputWithOptions(from_node_key, inputs, {});
}

WorkflowNode& WorkflowNode::AddInputWithOptions(
    const std::string& from_node_key,
    const std::vector<FieldMapping>& inputs,
    const WorkflowAddInputOptions& options) {
    
    WorkflowInputInfo info;
    info.from_node_key = from_node_key;
    info.mappings = inputs;
    info.options = options;
    
    add_inputs_.push_back(info);
    return *this;
}

WorkflowNode& WorkflowNode::AddDependency(const std::string& from_node_key) {
    WorkflowAddInputOptions opts;
    opts.dependency_without_input = true;
    return AddInputWithOptions(from_node_key, {}, opts);
}

WorkflowNode& WorkflowNode::SetStaticValue(
    const FieldPath& path, 
    const json& value) {
    
    std::string path_str = JoinFieldPath(path);
    static_values_[path_str] = value;
    return *this;
}

const std::string& WorkflowNode::GetKey() const {
    return key_;
}

const std::vector<WorkflowInputInfo>& WorkflowNode::GetAddInputs() const {
    return add_inputs_;
}

const std::map<std::string, json>& WorkflowNode::GetStaticValues() const {
    return static_values_;
}

void WorkflowNode::ClearAddInputs() {
    add_inputs_.clear();
}

bool WorkflowNode::CheckAndAddMappedPath(const std::vector<FieldPath>& paths) {
    // Check if entire output has been mapped
    auto it = mapped_field_path_.find("");
    if (it != mapped_field_path_.end()) {
        if (it->second.is_null() || it->second.is_object()) {
            // Already mapped entire output
            if (paths.empty()) {
                return false; // Conflict: trying to map entire output again
            }
        }
    } else {
        if (paths.empty()) {
            // Map entire output
            mapped_field_path_[""] = json(nullptr);
            return true;
        } else {
            mapped_field_path_[""] = json::object();
        }
    }
    
    // Check individual paths
    for (const auto& target_path : paths) {
        json* current = &mapped_field_path_[""];
        FieldPath traversed;
        
        for (size_t i = 0; i < target_path.size(); ++i) {
            const auto& path_segment = target_path[i];
            traversed.push_back(path_segment);
            
            if (!current->is_object()) {
                current->clear();
                *current = json::object();
            }
            
            if (current->contains(path_segment)) {
                auto& value = (*current)[path_segment];
                if (value.is_null()) {
                    // Conflict: terminal path already mapped
                    return false;
                }
            }
            
            if (i < target_path.size() - 1) {
                (*current)[path_segment] = json::object();
                current = &(*current)[path_segment];
            } else {
                (*current)[path_segment] = json(nullptr);
            }
        }
    }
    
    return true;
}

// WorkflowBranch implementation
WorkflowBranch::WorkflowBranch(
    const std::string& from_node_key,
    std::shared_ptr<GraphBranch> branch)
    : from_node_key_(from_node_key), 
      branch_(branch) {}

WorkflowBranch::~WorkflowBranch() = default;

const std::string& WorkflowBranch::GetFromNodeKey() const {
    return from_node_key_;
}

std::shared_ptr<GraphBranch> WorkflowBranch::GetBranch() const {
    return branch_;
}

// FieldMapping implementation
FieldMapping::FieldMapping() = default;

FieldMapping::FieldMapping(
    const FieldPath& from_path,
    const FieldPath& to_path)
    : from_path_(from_path), to_path_(to_path) {}

FieldMapping::~FieldMapping() = default;

void FieldMapping::SetFromNodeKey(const std::string& key) {
    from_node_key_ = key;
}

const std::string& FieldMapping::GetFromNodeKey() const {
    return from_node_key_;
}

const FieldPath& FieldMapping::GetFromPath() const {
    return from_path_;
}

const FieldPath& FieldMapping::GetToPath() const {
    return to_path_;
}

FieldPath FieldMapping::TargetPath() const {
    return to_path_;
}

// Helper functions
FieldMapping MapFields(const FieldPath& from, const FieldPath& to) {
    return FieldMapping(from, to);
}

FieldPath SplitFieldPath(const std::string& path) {
    if (path.empty()) {
        return {};
    }
    
    FieldPath result;
    std::stringstream ss(path);
    std::string segment;
    
    while (std::getline(ss, segment, '.')) {
        if (!segment.empty()) {
            result.push_back(segment);
        }
    }
    
    return result;
}

std::string JoinFieldPath(const FieldPath& path) {
    if (path.empty()) {
        return "";
    }
    
    std::string result;
    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) {
            result += ".";
        }
        result += path[i];
    }
    
    return result;
}

FieldPath ToFieldPath(const FieldPath& path) {
    return path;
}

// WorkflowAddInputOptions implementation
WorkflowAddInputOptions::WorkflowAddInputOptions()
    : no_direct_dependency(false),
      dependency_without_input(false) {}

WorkflowAddInputOptions WithNoDirectDependency() {
    WorkflowAddInputOptions opts;
    opts.no_direct_dependency = true;
    return opts;
}

// Dependency type helpers
std::string DependencyTypeToString(DependencyType type) {
    switch (type) {
        case DependencyType::Normal:
            return "Normal";
        case DependencyType::NoDirectDependency:
            return "NoDirectDependency";
        case DependencyType::Branch:
            return "Branch";
        default:
            return "Unknown";
    }
}

// WorkflowInputInfo implementation
WorkflowInputInfo::WorkflowInputInfo() = default;

WorkflowInputInfo::~WorkflowInputInfo() = default;

} // namespace compose
} // namespace eino
