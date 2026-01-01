/*
 * Copyright 2025 CloudWeGo Authors
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

#ifndef EINO_CPP_COMPOSE_GRAPH_RUN_H_
#define EINO_CPP_COMPOSE_GRAPH_RUN_H_

// Aligns with: eino/compose/graph_run.go
// Contains: runner struct and execution logic

#include <memory>
#include <map>
#include <vector>
#include <string>
#include <functional>
#include <exception>
#include <any>

// Forward declare json from nlohmann
namespace nlohmann {
    class json;
}

namespace eino {
namespace compose {

// Forward declarations
class Context;
class Channel;
class ChannelManager;
class TaskManager;
class CheckPointStore;
class CheckPointer;
struct Option;
struct CheckPoint;
template<typename I, typename O> class Graph;

// StateModifier type - aligns with checkpoint.h but forward declared here
using StateModifier = std::function<std::string(
    std::shared_ptr<Context> ctx,
    const std::vector<std::string>& path,
    nlohmann::json& state)>;

// =============================================================================
// Task Status and Representation
// Aligns with: eino/compose/graph_manager.go:232 (task struct)
// =============================================================================

enum class TaskStatus {
    Pending = 0,
    Queued = 1,
    Running = 2,
    Completed = 3,
    Failed = 4,
    Cancelled = 5
};

struct Task {
    std::string node_key;
    std::shared_ptr<void> input;
    std::shared_ptr<void> output;
    std::shared_ptr<std::exception> error;
    std::shared_ptr<Context> context;
    std::vector<std::shared_ptr<void>> options;
    TaskStatus status = TaskStatus::Pending;
    bool skip_pre_handler = false;
    int execution_order = -1;
    
    // ⭐ NEW: Reference to the GraphNode for execution
    // Aligns with Go's task.call.action (which contains the runnable)
    std::shared_ptr<GraphNode> graph_node;
    
    // ⭐ NEW: Method to use for execution
    // Determined by smart method selection logic
    std::string execution_method;  // "Invoke", "Stream", "Collect", "Transform"
    
    Task() = default;
    explicit Task(const std::string& key) : node_key(key) {}
};

// =============================================================================
// Graph Run Options
// Aligns with: eino/compose/graph_compile_options.go:19-36
// =============================================================================

enum class GraphRunType {
    DAG,     // Single pass, acyclic execution
    Pregel   // Iterative, supports cycles
};

// Forward declarations for options
struct FanInMergeConfig;

struct GraphRunOptions {
    int max_run_steps = 100;
    GraphRunType run_type = GraphRunType::DAG;
    bool eager_execution = true;
    long timeout_ms = 0;
    
    // Interrupt configuration - aligns with Go's interruptBeforeNodes/interruptAfterNodes
    std::vector<std::string> interrupt_before_nodes;
    std::vector<std::string> interrupt_after_nodes;
    
    // CheckPoint configuration (using raw pointer to avoid forward declaration issue)
    CheckPointStore* checkpoint_store = nullptr;
    // std::shared_ptr<Serializer> serializer;  // TODO: Add when Serializer is implemented
    
    // Graph metadata
    std::string graph_name;
    
    // Merge configurations for fan-in nodes
    // std::map<std::string, FanInMergeConfig> merge_configs;  // TODO: Add when FanInMergeConfig is complete
    
    // Eager execution control
    bool eager_disabled = false;
    
    GraphRunOptions() = default;
};

// =============================================================================
// CheckPoint for Interrupt/Resume
// Aligns with: eino/compose/checkpoint.go
// =============================================================================

struct CheckPoint {
    std::map<std::string, std::shared_ptr<Channel>> channels;
    std::map<std::string, std::shared_ptr<void>> inputs;
    std::map<std::string, bool> skip_pre_handler;
    std::shared_ptr<void> state;
    std::vector<std::string> rerun_nodes;
    std::map<std::string, std::shared_ptr<CheckPoint>> sub_graphs;
    std::map<std::string, std::map<std::string, std::string>> tools_node_executed_tools;
    
    CheckPoint() = default;
    
    bool IsValid() const {
        return !channels.empty() || !inputs.empty();
    }
};

// =============================================================================
// CheckPoint Store Interface
// =============================================================================

class CheckPointStore {
public:
    virtual ~CheckPointStore() = default;
    virtual bool Set(const std::string& id, const std::shared_ptr<CheckPoint>& cp) = 0;
    virtual std::shared_ptr<CheckPoint> Get(const std::string& id) = 0;
    virtual bool Delete(const std::string& id) = 0;
};

// =============================================================================
// Interrupt Errors
// Aligns with: eino/compose/interrupt.go
// =============================================================================

// Forward declaration
struct InterruptInfo;

// Interrupt temporary info for collecting interrupt state
// Aligns with: eino/compose/graph_run.go:403-411
struct InterruptTempInfo {
    std::map<std::string, std::shared_ptr<InterruptInfo>> sub_graph_interrupts;
    std::vector<std::string> interrupt_rerun_nodes;
    std::vector<std::string> interrupt_before_nodes;
    std::vector<std::string> interrupt_after_nodes;
    std::map<std::string, std::shared_ptr<void>> interrupt_rerun_extra;
    std::map<std::string, std::map<std::string, std::string>> interrupt_executed_tools;
    
    InterruptTempInfo() = default;
};

// InterruptAndRerunError for node-level interrupt and rerun
class InterruptAndRerunError : public std::exception {
public:
    explicit InterruptAndRerunError(std::shared_ptr<void> extra = nullptr)
        : extra_(extra) {}
    
    const char* what() const noexcept override {
        return "Node interrupted and requires rerun";
    }
    
    std::shared_ptr<void> GetExtra() const {
        return extra_;
    }
    
private:
    std::shared_ptr<void> extra_;
};

struct InterruptInfo {
    std::shared_ptr<void> state;
    std::vector<std::string> before_nodes;
    std::vector<std::string> after_nodes;
    std::vector<std::string> rerun_nodes;
    std::map<std::string, std::shared_ptr<void>> rerun_nodes_extra;
    std::map<std::string, std::shared_ptr<InterruptInfo>> sub_graphs;
    
    InterruptInfo() = default;
};

class InterruptError : public std::exception {
public:
    // 🔴 FIX: 添加消息参数，对齐 Go 版本
    explicit InterruptError(const std::string& message, std::shared_ptr<InterruptInfo> info)
        : info_(info), message_(message) {}
    
    // 保留向后兼容的构造函数
    explicit InterruptError(std::shared_ptr<InterruptInfo> info)
        : info_(info), message_("Graph execution interrupted") {}
    
    const char* what() const noexcept override {
        return message_.c_str();
    }
    
    std::shared_ptr<InterruptInfo> GetInfo() const {
        return info_;
    }
    
protected:
    std::shared_ptr<InterruptInfo> info_;
    std::string message_;
};

class SubGraphInterruptError : public InterruptError {
public:
    // 🔴 FIX: 添加消息参数，对齐 Go 版本
    SubGraphInterruptError(
        const std::string& message,
        std::shared_ptr<InterruptInfo> info,
        std::shared_ptr<CheckPoint> checkpoint)
        : InterruptError(message, info), checkpoint_(checkpoint) {}
    
    // 保留向后兼容的构造函数
    SubGraphInterruptError(
        std::shared_ptr<InterruptInfo> info,
        std::shared_ptr<CheckPoint> checkpoint)
        : InterruptError("Subgraph execution interrupted", info), checkpoint_(checkpoint) {}
    
    std::shared_ptr<CheckPoint> GetCheckPoint() const {
        return checkpoint_;
    }
    
private:
    std::shared_ptr<CheckPoint> checkpoint_;
};

// =============================================================================
// Graph Runner - Main Execution Engine
// Aligns with: eino/compose/graph_run.go:39-91 (runner struct)
// =============================================================================

template<typename I, typename O>
class GraphRunner {
public:
    GraphRunner(
        std::shared_ptr<Graph<I, O>> graph,
        const GraphRunOptions& opts = GraphRunOptions());
    
    ~GraphRunner() = default;
    
    // Main execution method
    // Aligns with: eino/compose/graph_run.go:93-103 (run method)
    O Run(
        std::shared_ptr<Context> ctx,
        const I& input,
        const std::vector<Option>& options = {});
    
    // Set run context initializer for state management
    // Aligns with: eino/compose/graph.go:808-811
    void SetRunCtx(std::function<std::shared_ptr<Context>(std::shared_ptr<Context>)> run_ctx) {
        run_ctx_ = run_ctx;
    }
    
    // Get the underlying graph
    std::shared_ptr<Graph<I, O>> GetGraph() const {
        return graph_;
    }
    
    // Get step count
    int GetStepCount() const {
        return step_count_;
    }
    
private:
    // Initialize channel manager
    // Aligns with: eino/compose/graph_run.go:777
    std::shared_ptr<ChannelManager> InitChannelManager(bool is_stream);
    
    // Initialize task manager
    // Aligns with: eino/compose/graph_run.go:766
    std::shared_ptr<TaskManager> InitTaskManager();
    
    // Calculate next tasks to execute
    // Aligns with: eino/compose/graph_run.go:648-680
    // Returns: tuple<nextTasks, result, isEnd, error>
    std::tuple<std::vector<std::shared_ptr<Task>>, O, bool, std::string>
    CalculateNextTasks(
        std::shared_ptr<Context> ctx,
        const std::vector<std::shared_ptr<Task>>& completed_tasks,
        bool is_stream,
        std::shared_ptr<ChannelManager> cm,
        const std::map<std::string, std::vector<std::any>>& opt_map);
    
    // Create tasks from node map
    // Aligns with: eino/compose/graph_run.go:682-700
    std::vector<std::shared_ptr<Task>> CreateTasks(
        std::shared_ptr<Context> ctx,
        const std::map<std::string, std::shared_ptr<void>>& node_map);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ⭐ Smart Method Selection Helpers
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    // Node capabilities structure
    struct NodeCapabilities {
        bool has_invoke = false;
        bool has_stream = false;
        bool has_collect = false;
        bool has_transform = false;
    };
    
    // Determine execution method based on input type, node capabilities, and downstream
    // Aligns with: eino/compose/graph_run.go:124-125 (runWrapper selection logic)
    std::string DetermineExecutionMethod(
        std::shared_ptr<void> input,
        std::shared_ptr<GraphNode> graph_node,
        const std::string& node_name);
    
    // Check if input is a StreamReader
    bool IsStreamInput(std::shared_ptr<void> input);
    
    // Detect node capabilities (which methods it supports)
    NodeCapabilities DetectNodeCapabilities(std::shared_ptr<GraphNode> graph_node);
    
    // Check if downstream nodes expect stream input
    bool CheckDownstreamExpectsStream(const std::string& node_name);
    
    // Resolve completed tasks and update channels
    // Aligns with: eino/compose/graph_run.go:702-764
    void ResolveCompletedTasks(
        std::shared_ptr<Context> ctx,
        const std::vector<std::shared_ptr<Task>>& completed_tasks,
        bool is_stream,
        std::shared_ptr<ChannelManager> cm,
        std::map<std::string, std::map<std::string, std::shared_ptr<void>>>& values,
        std::map<std::string, std::vector<std::string>>& controls);
    
    // Restore from checkpoint
    // Aligns with: eino/compose/graph_run.go:366-399
    std::tuple<std::shared_ptr<Context>, std::vector<std::shared_ptr<Task>>, std::string>
    RestoreFromCheckPoint(
        std::shared_ptr<Context> ctx,
        const std::vector<std::string>& path,
        StateModifier state_modifier,
        std::shared_ptr<CheckPoint> cp,
        bool is_stream,
        std::shared_ptr<ChannelManager> cm,
        const std::map<std::string, std::vector<std::any>>& opt_map);
    
    // Resolve interrupt information from completed tasks
    // Aligns with: eino/compose/graph_run.go:418-451
    void ResolveInterruptCompletedTasks(
        std::shared_ptr<InterruptTempInfo> temp_info,
        const std::vector<std::shared_ptr<Task>>& completed_tasks);
    
    // Get hit keys from tasks
    // Aligns with: eino/compose/graph_run.go:453-463
    std::vector<std::string> GetHitKeys(
        const std::vector<std::shared_ptr<Task>>& tasks,
        const std::vector<std::string>& target_keys);
    
    // Handle interrupt by saving checkpoint
    // Aligns with: eino/compose/graph_run.go:465-512
    // 🔴 FIX: 添加 is_sub_graph 和 checkpoint_id 参数
    InterruptError HandleInterrupt(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<InterruptTempInfo> temp_info,
        const std::vector<std::shared_ptr<Task>>& next_tasks,
        const std::map<std::string, std::shared_ptr<Channel>>& channels,
        bool is_stream,
        bool is_sub_graph = false,
        const std::string* checkpoint_id = nullptr);
    
    // Handle interrupt with subgraph and rerun nodes
    // Aligns with: eino/compose/graph_run.go:514-612
    // 🔴 FIX: 添加 checkpoint_id 和 is_sub_graph 参数，调整 cm 位置
    InterruptError HandleInterruptWithSubGraphAndRerunNodes(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<InterruptTempInfo> temp_info,
        const std::vector<std::shared_ptr<Task>>& all_tasks,
        const std::string* checkpoint_id,
        bool is_sub_graph,
        std::shared_ptr<ChannelManager> cm,
        bool is_stream);
    
    // Extract checkpoint info from options
    // Aligns with: eino/compose/graph_run.go:654-670
    struct CheckPointInfo {
        std::string checkpoint_id;
        std::string write_to_checkpoint_id;
        StateModifier state_modifier;
        bool force_new_run = false;
    };
    
    CheckPointInfo GetCheckPointInfo(const std::vector<Option>& options);
    
    std::shared_ptr<Graph<I, O>> graph_;
    GraphRunOptions options_;
    int step_count_ = 0;
    
    // Run context initializer for state management
    // Aligns with: eino/compose/graph_run.go:54
    std::function<std::shared_ptr<Context>(std::shared_ptr<Context>)> run_ctx_;
    
    // Interrupt configuration extracted from options
    // Aligns with: eino/compose/graph_run.go:78-79
    std::vector<std::string> interrupt_before_nodes_;
    std::vector<std::string> interrupt_after_nodes_;
    
    // CheckPoint store and pointer
    // Aligns with: eino/compose/graph_run.go:77
    CheckPointStore* checkpoint_store_ = nullptr;
    std::shared_ptr<CheckPointer> check_pointer_;
};

// =============================================================================
// Factory Functions
// =============================================================================

std::shared_ptr<CheckPointStore> CreateMemoryCheckPointStore();

// =============================================================================
// Factory Function for GraphRunner
// =============================================================================

// NewGraphRunner creates a GraphRunner instance
// Aligns with: eino/compose/graph.go:779-850 (runner creation in compile)
// 
// Usage:
//   auto graph = std::make_shared<Graph<Input, Output>>();
//   // ... add nodes and edges ...
//   graph->Compile();
//   auto runner = NewGraphRunner(graph, opts);
//   auto result = runner->Run(ctx, input);
//
template<typename I, typename O>
std::shared_ptr<GraphRunner<I, O>> NewGraphRunner(
    std::shared_ptr<Graph<I, O>> graph,
    const GraphRunOptions& opts = GraphRunOptions()) {
    
    if (!graph) {
        throw std::invalid_argument("Graph cannot be null");
    }
    
    if (!graph->IsCompiled()) {
        throw std::runtime_error("Graph must be compiled before creating runner. Call Compile() first.");
    }
    
    return std::make_shared<GraphRunner<I, O>>(graph, opts);
}

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_GRAPH_RUN_H_
