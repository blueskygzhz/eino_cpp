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

#include "eino/compose/graph_node.h"

#include <stdexcept>

namespace eino {
namespace compose {

std::shared_ptr<ComposableRunnable> GraphNode::CompileIfNeeded(const Context& ctx) {
    std::shared_ptr<ComposableRunnable> r;
    
    if (g_) {
        // Compile the graph
        auto cr = g_->Compile(ctx, node_info_->compile_option.get());
        if (!cr) {
            throw std::runtime_error("failed to compile graph");
        }
        r = cr;
        cr_ = cr;
    } else if (cr_) {
        r = cr_;
    } else {
        throw std::runtime_error("no graph or component provided");
    }
    
    // Set metadata
    r->SetMeta(executor_meta_);
    r->SetNodeInfo(node_info_);
    
    // Wrap with output key if specified
    if (!node_info_->output_key.empty()) {
        r = OutputKeyedComposableRunnable(node_info_->output_key, r);
    }
    
    // Wrap with input key if specified
    if (!node_info_->input_key.empty()) {
        r = InputKeyedComposableRunnable(node_info_->input_key, r);
    }
    
    return r;
}

ExecutorMeta ParseExecutorInfoFromComponent(Component c, const Any& executor) {
    ExecutorMeta meta;
    meta.component = c;
    
    // Try to get component type information
    // In C++, we'll use type_info or custom type registry
    // For now, use a simple string representation
    meta.component_impl_type = executor.type().name();
    
    // Check if component has callback support
    // This would require component introspection in C++
    meta.is_component_callback_enabled = false;
    
    return meta;
}

std::shared_ptr<NodeInfo> GetNodeInfo(const std::vector<GraphAddNodeOpt>& opts) {
    auto info = std::make_shared<NodeInfo>();
    
    // Parse options to fill NodeInfo
    for (const auto& opt : opts) {
        // Apply each option to NodeInfo
        // This requires implementing the GraphAddNodeOpt application logic
        // For now, return empty info
    }
    
    return info;
}

// Input key wrapper implementation
class InputKeyedRunnable : public ComposableRunnable {
public:
    InputKeyedRunnable(const std::string& key, std::shared_ptr<ComposableRunnable> inner)
        : key_(key), inner_(std::move(inner)) {}
    
    Any Invoke(const Context& ctx, const Any& input) override {
        // Extract value from map using key
        auto input_map = std::any_cast<std::map<std::string, Any>>(input);
        auto it = input_map.find(key_);
        if (it == input_map.end()) {
            throw std::runtime_error("input key not found: " + key_);
        }
        return inner_->Invoke(ctx, it->second);
    }
    
    StreamReader Stream(const Context& ctx, const Any& input) override {
        auto input_map = std::any_cast<std::map<std::string, Any>>(input);
        auto it = input_map.find(key_);
        if (it == input_map.end()) {
            throw std::runtime_error("input key not found: " + key_);
        }
        return inner_->Stream(ctx, it->second);
    }

private:
    std::string key_;
    std::shared_ptr<ComposableRunnable> inner_;
};

// Output key wrapper implementation
class OutputKeyedRunnable : public ComposableRunnable {
public:
    OutputKeyedRunnable(const std::string& key, std::shared_ptr<ComposableRunnable> inner)
        : key_(key), inner_(std::move(inner)) {}
    
    Any Invoke(const Context& ctx, const Any& input) override {
        auto output = inner_->Invoke(ctx, input);
        std::map<std::string, Any> result;
        result[key_] = output;
        return result;
    }
    
    StreamReader Stream(const Context& ctx, const Any& input) override {
        auto output_stream = inner_->Stream(ctx, input);
        // Wrap stream to add key to each chunk
        // This requires stream transformation implementation
        return output_stream;
    }

private:
    std::string key_;
    std::shared_ptr<ComposableRunnable> inner_;
};

std::shared_ptr<ComposableRunnable> InputKeyedComposableRunnable(
    const std::string& key,
    std::shared_ptr<ComposableRunnable> inner) {
    return std::make_shared<InputKeyedRunnable>(key, std::move(inner));
}

std::shared_ptr<ComposableRunnable> OutputKeyedComposableRunnable(
    const std::string& key,
    std::shared_ptr<ComposableRunnable> inner) {
    return std::make_shared<OutputKeyedRunnable>(key, std::move(inner));
}

} // namespace compose
} // namespace eino
