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

#include "eino/compose/graph_methods.h"
#include "eino/compose/graph.h"
#include "eino/compose/component_to_graph_node.h"
#include "eino/compose/tool_node.h"
#include "eino/compose/branch.h"
#include "eino/components/model.h"
#include "eino/schema/message.h"

namespace eino {
namespace compose {

// GraphExtended: Template specializations for common types
// These provide explicit implementations of the Graph methods for common input/output types
// Aligns with eino/compose/graph.go component node additions

// Specialization for Message input/output (most common for ChatModel)
// Aligns with eino/compose/graph.go:350-353 (AddChatModelNode)
template<>
void Graph<std::vector<schema::Message>, schema::Message>::AddChatModelNodeTyped(
    const std::string& key,
    std::shared_ptr<components::BaseChatModel> chat_model,
    const std::vector<GraphAddNodeOpt>& opts) {
    
    if (!chat_model) {
        throw std::invalid_argument("ChatModel cannot be null");
    }
    
    // Convert ChatModel component to GraphNode
    // The ChatModel has Generate(vector<Message>) -> Message
    // and Stream(vector<Message>) -> StreamReader<Message>
    
    // Create a wrapper runnable that calls the ChatModel methods
    class ChatModelRunnable : public Runnable<std::vector<schema::Message>, schema::Message> {
    public:
        explicit ChatModelRunnable(std::shared_ptr<components::BaseChatModel> model)
            : model_(model) {}
        
        schema::Message Invoke(
            std::shared_ptr<Context> ctx,
            const std::vector<schema::Message>& input,
            const std::vector<Option>& opts = {}) override {
            return model_->Generate(ctx, input, opts);
        }
        
        std::shared_ptr<StreamReader<schema::Message>> Stream(
            std::shared_ptr<Context> ctx,
            const std::vector<schema::Message>& input,
            const std::vector<Option>& opts = {}) override {
            return model_->Stream(ctx, input, opts);
        }
        
        const std::type_info& GetInputType() const override {
            return typeid(std::vector<schema::Message>);
        }
        
        const std::type_info& GetOutputType() const override {
            return typeid(schema::Message);
        }
        
    private:
        std::shared_ptr<components::BaseChatModel> model_;
    };
    
    auto runnable = std::make_shared<ChatModelRunnable>(chat_model);
    
    // Parse node options
    NodeTriggerMode trigger_mode = NodeTriggerMode::AllPredecessor;
    std::shared_ptr<NodeProcessor> processor = nullptr;
    
    // Add the node using base AddNode method
    AddNode(key, runnable, trigger_mode, processor);
}

// AddToolsNodeTyped for Message input/output
// Aligns with eino/compose/graph.go:375-378 (AddToolsNode)
template<>
void Graph<schema::Message, std::vector<schema::Message>>::AddToolsNodeTyped(
    const std::string& key,
    std::shared_ptr<ToolsNode> tools_node,
    const std::vector<GraphAddNodeOpt>& opts) {
    
    if (!tools_node) {
        throw std::invalid_argument("ToolsNode cannot be null");
    }
    
    // ToolsNode has Invoke(Message) -> vector<Message>
    // Wrap it as a Runnable
    class ToolsNodeRunnable : public Runnable<schema::Message, std::vector<schema::Message>> {
    public:
        explicit ToolsNodeRunnable(std::shared_ptr<ToolsNode> node)
            : node_(node) {}
        
        std::vector<schema::Message> Invoke(
            std::shared_ptr<Context> ctx,
            const schema::Message& input,
            const std::vector<Option>& opts = {}) override {
            return node_->Invoke(ctx, input);
        }
        
        std::shared_ptr<StreamReader<std::vector<schema::Message>>> Stream(
            std::shared_ptr<Context> ctx,
            const schema::Message& input,
            const std::vector<Option>& opts = {}) override {
            return node_->Stream(ctx, input);
        }
        
        const std::type_info& GetInputType() const override {
            return typeid(schema::Message);
        }
        
        const std::type_info& GetOutputType() const override {
            return typeid(std::vector<schema::Message>);
        }
        
    private:
        std::shared_ptr<ToolsNode> node_;
    };
    
    auto runnable = std::make_shared<ToolsNodeRunnable>(tools_node);
    
    NodeTriggerMode trigger_mode = NodeTriggerMode::AllPredecessor;
    std::shared_ptr<NodeProcessor> processor = nullptr;
    
    AddNode(key, runnable, trigger_mode, processor);
}

// AddBranchTyped for graph branching
// Aligns with eino/compose/graph.go:444-447 (AddBranch)
template<typename I, typename O>
void Graph<I, O>::AddBranchTyped(
    const std::string& start_node,
    std::shared_ptr<GraphBranch> branch) {
    
    if (is_compiled_) {
        throw std::runtime_error("Graph already compiled, cannot add branch");
    }
    
    if (start_node.empty()) {
        throw std::invalid_argument("Start node cannot be empty");
    }
    
    if (!nodes_.count(start_node) && start_node != START_NODE) {
        throw std::runtime_error("Start node not found: " + start_node);
    }
    
    if (!branch) {
        throw std::invalid_argument("Branch cannot be null");
    }
    
    // Validate branch end nodes
    for (const auto& end_node : branch->GetEndNodes()) {
        if (!nodes_.count(end_node) && end_node != END_NODE) {
            throw std::runtime_error("Branch end node not found: " + end_node);
        }
    }
    
    // Store branch
    branches_[start_node].push_back(branch);
}

// Explicit template instantiations for common types
// This ensures the linker can find these template implementations

// Message-based graphs (most common for ChatModel + Tools)
template class Graph<std::vector<schema::Message>, schema::Message>;
template class Graph<schema::Message, std::vector<schema::Message>>;
template class Graph<schema::Message, schema::Message>;

// String-based graphs
template class Graph<std::string, std::string>;
template class Graph<std::vector<std::string>, std::string>;

// Generic graphs
template class Graph<void*, void*>;

} // namespace compose
} // namespace eino
