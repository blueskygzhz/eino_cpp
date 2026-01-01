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

#ifndef EINO_CPP_COMPOSE_TYPES_H_
#define EINO_CPP_COMPOSE_TYPES_H_

#include <string>

namespace eino {
namespace compose {

// Component type constants for built-in components
enum class ComponentType {
    Unknown = 0,
    Graph = 1,
    Workflow = 2,
    Chain = 3,
    Passthrough = 4,
    ToolsNode = 5,
    Lambda = 6,
};

// Convert ComponentType to string
inline std::string ComponentTypeToString(ComponentType type) {
    switch (type) {
        case ComponentType::Unknown: return "Unknown";
        case ComponentType::Graph: return "Graph";
        case ComponentType::Workflow: return "Workflow";
        case ComponentType::Chain: return "Chain";
        case ComponentType::Passthrough: return "Passthrough";
        case ComponentType::ToolsNode: return "ToolsNode";
        case ComponentType::Lambda: return "Lambda";
        default: return "Unknown";
    }
}

// Node trigger mode controls how nodes are triggered in a graph
enum class NodeTriggerMode {
    // AnyPredecessor: node triggers when any predecessor completes
    AnyPredecessor = 0,
    // AllPredecessor: node triggers only when all predecessors complete
    AllPredecessor = 1,
};

// Convert NodeTriggerMode to string
inline std::string NodeTriggerModeToString(NodeTriggerMode mode) {
    switch (mode) {
        case NodeTriggerMode::AnyPredecessor: return "any_predecessor";
        case NodeTriggerMode::AllPredecessor: return "all_predecessor";
        default: return "unknown";
    }
}

// Standard node names
constexpr const char* START_NODE = "start";
constexpr const char* END_NODE = "end";

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_TYPES_H_
