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

#include "eino/compose/branch.h"
#include <stdexcept>
#include <vector>
#include <map>
#include <string>

namespace eino {
namespace compose {

// GraphBranch implementation
GraphBranch::GraphBranch()
    : input_type_(typeid(void)), 
      idx_(0), 
      no_data_flow_(false) {}

GraphBranch::~GraphBranch() = default;

std::vector<std::string> GraphBranch::Invoke(
    std::shared_ptr<Context> ctx, 
    const json& input) {
    if (!invoke_func_) {
        throw std::runtime_error("GraphBranch: invoke function not set");
    }
    return invoke_func_(ctx, input);
}

std::vector<std::string> GraphBranch::Collect(
    std::shared_ptr<Context> ctx,
    std::shared_ptr<StreamReaderBase> input) {
    if (!collect_func_) {
        throw std::runtime_error("GraphBranch: collect function not set");
    }
    return collect_func_(ctx, input);
}

const std::type_info& GraphBranch::GetInputType() const {
    return input_type_;
}

const std::map<std::string, bool>& GraphBranch::GetEndNodes() const {
    return end_nodes_;
}

void GraphBranch::SetEndNodes(const std::map<std::string, bool>& end_nodes) {
    end_nodes_ = end_nodes;
}

void GraphBranch::SetIdx(int idx) {
    idx_ = idx;
}

int GraphBranch::GetIdx() const {
    return idx_;
}

void GraphBranch::SetNoDataFlow(bool no_data_flow) {
    no_data_flow_ = no_data_flow;
}

bool GraphBranch::GetNoDataFlow() const {
    return no_data_flow_;
}

void GraphBranch::SetInvokeFunc(
    std::function<std::vector<std::string>(
        std::shared_ptr<Context>, const json&)> func) {
    invoke_func_ = func;
}

void GraphBranch::SetCollectFunc(
    std::function<std::vector<std::string>(
        std::shared_ptr<Context>, 
        std::shared_ptr<StreamReaderBase>)> func) {
    collect_func_ = func;
}

void GraphBranch::SetInputType(const std::type_info& type) {
    input_type_ = type;
}

// Helper function implementations
namespace {

// Validate that returned end nodes are in the allowed set
std::vector<std::string> ValidateEndNodes(
    const std::vector<std::string>& returned_nodes,
    const std::map<std::string, bool>& allowed_nodes,
    const std::string& error_prefix) {
    
    std::vector<std::string> result;
    for (const auto& node : returned_nodes) {
        auto it = allowed_nodes.find(node);
        if (it == allowed_nodes.end()) {
            throw std::runtime_error(
                error_prefix + ": returned unintended end node: " + node);
        }
        result.push_back(node);
    }
    return result;
}

// Convert multi-branch result to single result
std::string MultiToSingle(
    const std::map<std::string, bool>& ends,
    const std::map<std::string, bool>& allowed_nodes,
    const std::string& error_prefix) {
    
    std::vector<std::string> result;
    for (const auto& pair : ends) {
        if (pair.second) {
            if (allowed_nodes.find(pair.first) == allowed_nodes.end()) {
                throw std::runtime_error(
                    error_prefix + ": returned unintended end node: " + pair.first);
            }
            result.push_back(pair.first);
        }
    }
    
    if (result.empty()) {
        throw std::runtime_error(error_prefix + ": no end node selected");
    }
    
    if (result.size() > 1) {
        throw std::runtime_error(
            error_prefix + ": multiple end nodes selected for single branch");
    }
    
    return result[0];
}

} // namespace

// NewGraphMultiBranch creates a multi-choice graph branch
std::shared_ptr<GraphBranch> NewGraphMultiBranchImpl(
    std::function<std::map<std::string, bool>(
        std::shared_ptr<Context>, const json&)> condition,
    const std::map<std::string, bool>& end_nodes,
    const std::type_info& input_type) {
    
    auto branch = std::make_shared<GraphBranch>();
    branch->SetEndNodes(end_nodes);
    branch->SetInputType(input_type);
    
    // Set invoke function
    branch->SetInvokeFunc([condition, end_nodes](
        std::shared_ptr<Context> ctx, 
        const json& input) -> std::vector<std::string> {
        
        auto ends = condition(ctx, input);
        std::vector<std::string> result;
        
        for (const auto& pair : ends) {
            if (pair.second) {
                if (end_nodes.find(pair.first) == end_nodes.end()) {
                    throw std::runtime_error(
                        "branch invocation returns unintended end node: " + pair.first);
                }
                result.push_back(pair.first);
            }
        }
        
        return result;
    });
    
    return branch;
}

// NewStreamGraphMultiBranch creates a multi-choice stream graph branch
std::shared_ptr<GraphBranch> NewStreamGraphMultiBranchImpl(
    std::function<std::map<std::string, bool>(
        std::shared_ptr<Context>, 
        std::shared_ptr<StreamReaderBase>)> condition,
    const std::map<std::string, bool>& end_nodes,
    const std::type_info& input_type) {
    
    auto branch = std::make_shared<GraphBranch>();
    branch->SetEndNodes(end_nodes);
    branch->SetInputType(input_type);
    
    // Set collect function
    branch->SetCollectFunc([condition, end_nodes](
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReaderBase> input) -> std::vector<std::string> {
        
        auto ends = condition(ctx, input);
        std::vector<std::string> result;
        
        for (const auto& pair : ends) {
            if (pair.second) {
                if (end_nodes.find(pair.first) == end_nodes.end()) {
                    throw std::runtime_error(
                        "branch invocation returns unintended end node: " + pair.first);
                }
                result.push_back(pair.first);
            }
        }
        
        return result;
    });
    
    return branch;
}

// NewGraphBranch creates a single-choice graph branch
std::shared_ptr<GraphBranch> NewGraphBranchImpl(
    std::function<std::string(
        std::shared_ptr<Context>, const json&)> condition,
    const std::map<std::string, bool>& end_nodes,
    const std::type_info& input_type) {
    
    // Wrap single-choice condition as multi-choice
    auto multi_condition = [condition](
        std::shared_ptr<Context> ctx,
        const json& input) -> std::map<std::string, bool> {
        
        auto result = condition(ctx, input);
        return {{result, true}};
    };
    
    return NewGraphMultiBranchImpl(multi_condition, end_nodes, input_type);
}

// NewStreamGraphBranch creates a single-choice stream graph branch
std::shared_ptr<GraphBranch> NewStreamGraphBranchImpl(
    std::function<std::string(
        std::shared_ptr<Context>, 
        std::shared_ptr<StreamReaderBase>)> condition,
    const std::map<std::string, bool>& end_nodes,
    const std::type_info& input_type) {
    
    // Wrap single-choice condition as multi-choice
    auto multi_condition = [condition](
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReaderBase> input) -> std::map<std::string, bool> {
        
        auto result = condition(ctx, input);
        return {{result, true}};
    };
    
    return NewStreamGraphMultiBranchImpl(multi_condition, end_nodes, input_type);
}

} // namespace compose
} // namespace eino
