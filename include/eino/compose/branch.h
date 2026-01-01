/*
 * Copyright 2024 CloudWeGo Authors
 *
 * Licensed under the Apache License, Version 2.0 (the \"License\");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an \"AS IS\" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EINO_CPP_COMPOSE_BRANCH_H_
#define EINO_CPP_COMPOSE_BRANCH_H_

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <functional>
#include "stream_reader.h"

namespace eino {
namespace compose {

// GraphBranchCondition determines next node based on input
// Aligns with eino compose.GraphBranchCondition
// Go reference: eino/compose/branch.go line 28
template<typename T>
using GraphBranchCondition = std::function<std::string(void* ctx, const T& input)>;

// StreamGraphBranchCondition determines next node based on stream input
// Aligns with eino compose.StreamGraphBranchCondition
// Go reference: eino/compose/branch.go line 31
template<typename T>
using StreamGraphBranchCondition = std::function<
    std::string(void* ctx, std::shared_ptr<schema::StreamReader<T>> input)>;

// GraphMultiBranchCondition determines multiple next nodes
// Aligns with eino compose.GraphMultiBranchCondition
// Go reference: eino/compose/branch.go line 34
template<typename T>
using GraphMultiBranchCondition = std::function<
    std::set<std::string>(void* ctx, const T& input)>;

// StreamGraphMultiBranchCondition determines multiple next nodes from stream
// Aligns with eino compose.StreamGraphMultiBranchCondition
// Go reference: eino/compose/branch.go line 37
template<typename T>
using StreamGraphMultiBranchCondition = std::function<
    std::set<std::string>(void* ctx, std::shared_ptr<schema::StreamReader<T>> input)>;

// GraphBranch represents a conditional branch in the graph
// Aligns with eino compose.GraphBranch
// Go reference: eino/compose/branch.go lines 40-49
class GraphBranch {
public:
    virtual ~GraphBranch() = default;
    
    // Invoke executes the branch condition and returns next node(s)
    virtual std::vector<std::string> Invoke(void* ctx, const void* input) = 0;
    
    // Collect executes branch condition from stream input
    virtual std::vector<std::string> Collect(
        void* ctx, 
        std::shared_ptr<void> stream_input) = 0;
    
    // GetEndNodes returns all possible end nodes
    virtual std::set<std::string> GetEndNodes() const = 0;
    
    // GetInputType returns expected input type
    virtual const std::type_info& GetInputType() const = 0;
    
    // IsDataFlow checks if branch has data flow (vs control-only)
    virtual bool IsDataFlow() const { return true; }
    
    // GetIndex returns branch index for parallel branches
    virtual int GetIndex() const { return index_; }
    
    // SetIndex sets branch index
    virtual void SetIndex(int idx) { index_ = idx; }

protected:
    int index_ = 0;
    bool no_data_flow_ = false;
};

// ConcreteGraphBranch is templated implementation of GraphBranch
// Aligns with eino compose.newGraphBranch
// Go reference: eino/compose/branch.go lines 55-87
template<typename T>
class ConcreteGraphBranch : public GraphBranch {
public:
    ConcreteGraphBranch(
        std::function<std::vector<std::string>(void*, const T&)> invoke_func,
        std::function<std::vector<std::string>(void*, std::shared_ptr<schema::StreamReader<T>>)> collect_func,
        const std::set<std::string>& end_nodes)
        : invoke_func_(invoke_func)
        , collect_func_(collect_func)
        , end_nodes_(end_nodes) {}
    
    std::vector<std::string> Invoke(void* ctx, const void* input) override {
        if (!input) {
            throw std::runtime_error("Branch: null input");
        }
        const T* typed_input = static_cast<const T*>(input);
        return invoke_func_(ctx, *typed_input);
    }
    
    std::vector<std::string> Collect(
        void* ctx, 
        std::shared_ptr<void> stream_input) override {
        
        auto typed_stream = std::static_pointer_cast<schema::StreamReader<T>>(stream_input);
        if (!typed_stream) {
            throw std::runtime_error("Branch: invalid stream input type");
        }
        return collect_func_(ctx, typed_stream);
    }
    
    std::set<std::string> GetEndNodes() const override {
        return end_nodes_;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(T);
    }

private:
    std::function<std::vector<std::string>(void*, const T&)> invoke_func_;
    std::function<std::vector<std::string>(void*, std::shared_ptr<schema::StreamReader<T>>)> collect_func_;
    std::set<std::string> end_nodes_;
};

// NewGraphBranch creates a single-choice branch
// Aligns with eino compose.NewGraphBranch
// Go reference: eino/compose/branch.go lines 143-150
//
// Example:
//   auto condition = [](void* ctx, const std::string& in) {
//       if (in == "hello") return "path1";
//       return "path2";
//   };
//   std::set<std::string> ends = {"path1", "path2"};
//   auto branch = NewGraphBranch(condition, ends);
//   graph->AddBranch("node_before_branch", branch);
template<typename T>
std::shared_ptr<GraphBranch> NewGraphBranch(
    GraphBranchCondition<T> condition,
    const std::set<std::string>& end_nodes) {
    
    // Convert single-choice to multi-choice
    auto multi_cond = [condition, end_nodes](void* ctx, const T& input) 
        -> std::vector<std::string> {
        
        std::string result = condition(ctx, input);
        
        // Validate result is in end_nodes
        if (end_nodes.find(result) == end_nodes.end()) {
            throw std::runtime_error(
                "Branch condition returned unintended end node: " + result);
        }
        
        return std::vector<std::string>{result};
    };
    
    // Collect version (for stream input)
    auto collect_func = [multi_cond](
        void* ctx, 
        std::shared_ptr<schema::StreamReader<T>> stream) 
        -> std::vector<std::string> {
        
        // Read and merge stream, then apply condition
        // For now, simplified: read first chunk
        T value;
        if (stream && stream->Read(value)) {
            return multi_cond(ctx, value);
        }
        throw std::runtime_error("Branch: failed to read from stream");
    };
    
    return std::make_shared<ConcreteGraphBranch<T>>(
        multi_cond, collect_func, end_nodes);
}

// NewGraphMultiBranch creates a multi-choice branch (can route to multiple nodes)
// Aligns with eino compose.NewGraphMultiBranch
// Go reference: eino/compose/branch.go lines 89-108
template<typename T>
std::shared_ptr<GraphBranch> NewGraphMultiBranch(
    GraphMultiBranchCondition<T> condition,
    const std::set<std::string>& end_nodes) {
    
    auto invoke_func = [condition, end_nodes](void* ctx, const T& input)
        -> std::vector<std::string> {
        
        std::set<std::string> results = condition(ctx, input);
        std::vector<std::string> result_vec;
        
        // Validate all results are in end_nodes
        for (const auto& node : results) {
            if (end_nodes.find(node) == end_nodes.end()) {
                throw std::runtime_error(
                    "Branch condition returned unintended end node: " + node);
            }
            result_vec.push_back(node);
        }
        
        return result_vec;
    };
    
    auto collect_func = [invoke_func](
        void* ctx,
        std::shared_ptr<schema::StreamReader<T>> stream)
        -> std::vector<std::string> {
        
        T value;
        if (stream && stream->Read(value)) {
            return invoke_func(ctx, value);
        }
        throw std::runtime_error("Branch: failed to read from stream");
    };
    
    return std::make_shared<ConcreteGraphBranch<T>>(
        invoke_func, collect_func, end_nodes);
}

// NewStreamGraphBranch creates a single-choice branch for stream input
// Aligns with eino compose.NewStreamGraphBranch (derived from NewGraphBranch)
// Go reference: eino/compose/branch.go lines 143-150
template<typename T>
std::shared_ptr<GraphBranch> NewStreamGraphBranch(
    StreamGraphBranchCondition<T> condition,
    const std::set<std::string>& end_nodes) {
    
    auto invoke_func = [](void* ctx, const T& input) 
        -> std::vector<std::string> {
        throw std::runtime_error("StreamGraphBranch: Invoke not supported, use Stream mode");
    };
    
    auto collect_func = [condition, end_nodes](
        void* ctx,
        std::shared_ptr<schema::StreamReader<T>> stream)
        -> std::vector<std::string> {
        
        std::string result = condition(ctx, stream);
        
        if (end_nodes.find(result) == end_nodes.end()) {
            throw std::runtime_error(
                "Branch condition returned unintended end node: " + result);
        }
        
        return std::vector<std::string>{result};
    };
    
    return std::make_shared<ConcreteGraphBranch<T>>(
        invoke_func, collect_func, end_nodes);
}

// NewStreamGraphMultiBranch creates a multi-choice branch for stream input
// Aligns with eino compose.NewStreamGraphMultiBranch
// Go reference: eino/compose/branch.go lines 110-131
template<typename T>
std::shared_ptr<GraphBranch> NewStreamGraphMultiBranch(
    StreamGraphMultiBranchCondition<T> condition,
    const std::set<std::string>& end_nodes) {
    
    auto invoke_func = [](void* ctx, const T& input)
        -> std::vector<std::string> {
        throw std::runtime_error("StreamGraphMultiBranch: Invoke not supported, use Stream mode");
    };
    
    auto collect_func = [condition, end_nodes](
        void* ctx,
        std::shared_ptr<schema::StreamReader<T>> stream)
        -> std::vector<std::string> {
        
        std::set<std::string> results = condition(ctx, stream);
        std::vector<std::string> result_vec;
        
        for (const auto& node : results) {
            if (end_nodes.find(node) == end_nodes.end()) {
                throw std::runtime_error(
                    "Branch condition returned unintended end node: " + node);
            }
            result_vec.push_back(node);
        }
        
        return result_vec;
    };
    
    return std::make_shared<ConcreteGraphBranch<T>>(
        invoke_func, collect_func, end_nodes);
}

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_BRANCH_H_
