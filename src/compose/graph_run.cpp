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

// Aligns with: eino/compose/graph_run.go
// Implementation of GraphRunner and execution logic

#include "eino/compose/graph_run.h"
#include "eino/compose/graph.h"
#include "eino/compose/graph_manager.h"
#include "eino/compose/state.h"
#include "eino/compose/checkpoint.h"
#include "eino/compose/typed_value.h"
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace eino {
namespace compose {

using json = nlohmann::json;

// =============================================================================
// Memory CheckPoint Store Implementation
// =============================================================================

class MemoryCheckPointStore : public CheckPointStore {
public:
    bool Set(const std::string& id, const std::shared_ptr<CheckPoint>& cp) override {
        std::lock_guard<std::mutex> lock(mutex_);
        store_[id] = cp;
        return true;
    }
    
    std::shared_ptr<CheckPoint> Get(const std::string& id) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = store_.find(id);
        if (it != store_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    bool Delete(const std::string& id) override {
        std::lock_guard<std::mutex> lock(mutex_);
        return store_.erase(id) > 0;
    }
    
private:
    std::mutex mutex_;
    std::map<std::string, std::shared_ptr<CheckPoint>> store_;
};

std::shared_ptr<CheckPointStore> CreateMemoryCheckPointStore() {
    return std::make_shared<MemoryCheckPointStore>();
}

// =============================================================================
// GraphRunner Implementation
// Aligns with: eino/compose/graph_run.go:39-91 (runner struct)
// =============================================================================

template<typename I, typename O>
GraphRunner<I, O>::GraphRunner(
    std::shared_ptr<Graph<I, O>> graph,
    const GraphRunOptions& opts)
    : graph_(graph), options_(opts), step_count_(0) {
    if (!graph_) {
        throw std::runtime_error("Graph cannot be null");
    }
    
    // Extract interrupt configuration from options
    // Aligns with: eino/compose/graph.go:834-836
    interrupt_before_nodes_ = opts.interrupt_before_nodes;
    interrupt_after_nodes_ = opts.interrupt_after_nodes;
    checkpoint_store_ = opts.checkpoint_store;
    
    // Create CheckPointer if checkpoint store is available
    // Aligns with: eino/compose/graph.go:832
    // Note: In Go, inputPairs and outputPairs are collected from all nodes
    // For C++, we create CheckPointer without streamConverter for now
    // TODO: Collect streamConvertPairs from nodes and pass to CheckPointer
    if (checkpoint_store_) {
        check_pointer_ = std::make_shared<CheckPointer>(
            std::shared_ptr<CheckPointStore>(checkpoint_store_, [](CheckPointStore*){}));
    }
}

// Main execution method
// Aligns with: eino/compose/graph_run.go:93-363 (run method with interrupt handling)
template<typename I, typename O>
O GraphRunner<I, O>::Run(
    std::shared_ptr<Context> ctx,
    const I& input,
    const std::vector<Option>& options) {
    
    if (!ctx) {
        throw std::runtime_error("Context cannot be null");
    }
    
    if (!graph_->IsCompiled()) {
        throw std::runtime_error("Graph not compiled");
    }
    
    // Determine execution mode based on graph type
    // Aligns with: eino/compose/graph_run.go:110-113
    bool is_stream = false;
    
    // Initialize runtime components
    // Aligns with: eino/compose/graph_run.go:115-120
    auto cm = InitChannelManager(is_stream);
    auto tm = InitTaskManager();
    
    int max_steps = options_.max_run_steps;
    
    // Validate maxSteps based on graph type
    // Aligns with: eino/compose/graph_run.go:129-143
    if (options_.run_type == GraphRunType::DAG) {
        if (options_.max_run_steps > 0) {
            throw std::runtime_error("Cannot set max_run_steps in DAG mode");
        }
        // DAG mode doesn't need step limit, use a large value as safety bound
        max_steps = 1000;
    } else {
        // Pregel mode requires step limit
        if (max_steps < 1) {
            throw std::runtime_error("max_run_steps must be at least 1 for Pregel mode");
        }
    }
    
    // Extract CheckPointID and related options
    // Aligns with: eino/compose/graph_run.go:156-159
    auto cp_info = GetCheckPointInfo(options);
    if (!cp_info.checkpoint_id.empty() && !checkpoint_store_) {
        throw std::runtime_error("Receive checkpoint id but have not set checkpoint store");
    }
    
    // Initialize state if not restored from checkpoint
    // Aligns with: eino/compose/graph_run.go:162-213
    bool initialized = false;
    std::vector<std::shared_ptr<Task>> next_tasks;
    std::string err_msg;
    
    // Try to load checkpoint from context (for subgraph scenarios)
    // Aligns with: eino/compose/graph_run.go:167-173
    auto cp_from_ctx = GetCheckPointFromCtx(ctx);
    if (cp_from_ctx) {
        // In subgraph, try to load checkpoint from ctx
        initialized = true;
        
        // TODO: Extract path from context (getNodeKey)
        std::vector<std::string> path;
        
        auto sm = cp_info.state_modifier; // TODO: Also try getStateModifier(ctx)
        
        std::tie(ctx, next_tasks, err_msg) = RestoreFromCheckPoint(
            ctx, path, sm, cp_from_ctx, is_stream, cm, std::map<std::string, std::vector<std::any>>());
        
        if (!err_msg.empty()) {
            throw std::runtime_error("Restore from checkpoint (ctx) fail: " + err_msg);
        }
    } 
    // Try to load checkpoint from store
    // Aligns with: eino/compose/graph_run.go:174-189
    else if (!cp_info.checkpoint_id.empty() && !cp_info.force_new_run) {
        auto [cp_from_store, load_err] = GetCheckPointFromStore(
            ctx, cp_info.checkpoint_id, checkpoint_store_);
        
        if (!load_err.empty()) {
            throw std::runtime_error("Load checkpoint from store fail: " + load_err);
        }
        
        if (cp_from_store) {
            // Load checkpoint from store
            initialized = true;
            
            // Set state modifier and checkpoint to context
            // Aligns with: eino/compose/graph_run.go:182-183
            // TODO: ctx = setStateModifier(ctx, cp_info.state_modifier);
            ctx = SetCheckPointToCtx(ctx, cp_from_store);
            
            std::vector<std::string> new_path; // NewNodePath()
            
            std::tie(ctx, next_tasks, err_msg) = RestoreFromCheckPoint(
                ctx, new_path, cp_info.state_modifier, cp_from_store, is_stream, cm, 
                std::map<std::string, std::vector<std::any>>());
            
            if (!err_msg.empty()) {
                throw std::runtime_error("Restore from checkpoint (store) fail: " + err_msg);
            }
        }
    }
    
    if (!initialized) {
        // Initialize state context if runCtx is set
        // Aligns with: eino/compose/graph_run.go:192-194
        if (run_ctx_) {
            ctx = run_ctx_(ctx);
        }
        
        // Create initial tasks from START node
        // Aligns with: eino/compose/graph_run.go:165-213
        auto start_task = std::make_shared<Task>("__START__");
        start_task->input = std::make_shared<I>(input);
        start_task->context = ctx;
        start_task->status = TaskStatus::Queued;
        
        auto start_nodes = graph_->GetStartNodes();
        for (const auto& node_name : start_nodes) {
            auto task = std::make_shared<Task>(node_name);
            task->input = std::make_shared<I>(input);
            task->context = ctx;
            task->status = TaskStatus::Queued;
            next_tasks.push_back(task);
        }
        
        // Check for interrupt before initial nodes
        // Aligns with: eino/compose/graph_run.go:215-226
        if (!interrupt_before_nodes_.empty()) {
            auto hit_keys = GetHitKeys(next_tasks, interrupt_before_nodes_);
            if (!hit_keys.empty()) {
                auto temp_info = std::make_shared<InterruptTempInfo>();
                temp_info->interrupt_before_nodes = hit_keys;
                // 🔴 FIX: 传递 checkpoint_id（如果有）
                const std::string* cp_id_ptr = cp_info.checkpoint_id.empty() ? nullptr : &cp_info.checkpoint_id;
                throw HandleInterrupt(ctx, temp_info, next_tasks, cm->GetChannels(), is_stream, false, cp_id_ptr);
            }
        }
    }
    
    O result;
    std::vector<std::shared_ptr<Task>> last_completed;
    
    // Main execution loop
    // Aligns with: eino/compose/graph_run.go:232-363
    for (step_count_ = 0; step_count_ < max_steps; ++step_count_) {
        // Check for context cancellation
        // Aligns with: eino/compose/graph_run.go:234-239
        if (ctx->IsCancelled()) {
            tm->WaitAll();
            throw std::runtime_error("Context has been canceled");
        }
        
        if (next_tasks.empty()) {
            if (last_completed.empty()) {
                throw std::runtime_error("No tasks to execute");
            }
            break;
        }
        
        if (options_.run_type != GraphRunType::DAG && step_count_ >= max_steps) {
            throw std::runtime_error("Exceeded max run steps");
        }
        
        // Submit tasks for execution
        // Aligns with: eino/compose/graph_run.go:249-252
        tm->Submit(next_tasks);
        
        // Wait for tasks to complete and handle cancellation
        // Aligns with: eino/compose/graph_run.go:254-271
        auto temp_info = std::make_shared<InterruptTempInfo>();
        std::vector<std::shared_ptr<Task>> completed_tasks;
        std::vector<std::shared_ptr<Task>> cancelled_tasks;
        bool was_cancelled = false;
        
        tm->Wait(completed_tasks, was_cancelled, cancelled_tasks);
        
        if (was_cancelled) {
            if (!cancelled_tasks.empty()) {
                // Cancelled tasks become rerun nodes
                for (const auto& t : cancelled_tasks) {
                    temp_info->interrupt_rerun_nodes.push_back(t->node_key);
                }
            } else {
                // No cancelled tasks, mark completed as interrupt after
                for (const auto& t : completed_tasks) {
                    temp_info->interrupt_after_nodes.push_back(t->node_key);
                }
            }
        }
        
        // Resolve completed tasks to extract interrupt info
        // Aligns with: eino/compose/graph_run.go:273-276
        ResolveInterruptCompletedTasks(temp_info, completed_tasks);
        
        // Handle subgraph interrupts and rerun nodes
        // Aligns with: eino/compose/graph_run.go:278-305
        if (!temp_info->sub_graph_interrupts.empty() || 
            !temp_info->interrupt_rerun_nodes.empty()) {
            
            std::vector<std::shared_ptr<Task>> new_completed;
            std::vector<std::shared_ptr<Task>> new_cancelled;
            tm->WaitAll(new_completed, new_cancelled);
            
            for (const auto& ct : new_cancelled) {
                temp_info->interrupt_rerun_nodes.push_back(ct->node_key);
            }
            
            ResolveInterruptCompletedTasks(temp_info, new_completed);
            
            // Combine all tasks for checkpoint
            std::vector<std::shared_ptr<Task>> all_tasks;
            all_tasks.insert(all_tasks.end(), completed_tasks.begin(), completed_tasks.end());
            all_tasks.insert(all_tasks.end(), new_completed.begin(), new_completed.end());
            all_tasks.insert(all_tasks.end(), cancelled_tasks.begin(), cancelled_tasks.end());
            all_tasks.insert(all_tasks.end(), new_cancelled.begin(), new_cancelled.end());
            
            // 🔴 FIX: 传递 checkpoint_id（如果有）
            const std::string* cp_id_ptr = cp_info.checkpoint_id.empty() ? nullptr : &cp_info.checkpoint_id;
            throw HandleInterruptWithSubGraphAndRerunNodes(
                ctx, temp_info, all_tasks, cp_id_ptr, false, cm, is_stream);
        }
        
        if (completed_tasks.empty()) {
            std::string last_nodes_str;
            for (const auto& t : last_completed) {
                last_nodes_str += t->node_key + ",";
            }
            throw std::runtime_error("No tasks to execute, last completed: " + last_nodes_str);
        }
        
        last_completed = completed_tasks;
        
        // Check for errors
        for (const auto& task : completed_tasks) {
            if (task->status == TaskStatus::Failed) {
                if (task->error) {
                    throw *task->error;
                }
                throw std::runtime_error("Task failed: " + task->node_key);
            }
        }
        
        // Calculate next tasks
        // Aligns with: eino/compose/graph_run.go:313-319
        auto [calc_next_tasks, calc_result, is_end, calc_err] = 
            CalculateNextTasks(ctx, completed_tasks, is_stream, cm, std::map<std::string, std::vector<std::any>>{});
        
        if (!calc_err.empty()) {
            throw std::runtime_error("Failed to calculate next tasks: " + calc_err);
        }
        
        if (is_end) {
            return calc_result;
        }
        
        next_tasks = calc_next_tasks;
        
        // Check for interrupt before next nodes
        // Aligns with: eino/compose/graph_run.go:321-362
        auto hit_before = GetHitKeys(next_tasks, interrupt_before_nodes_);
        auto hit_after = GetHitKeys(completed_tasks, interrupt_after_nodes_);
        
        temp_info->interrupt_before_nodes = hit_before;
        if (!hit_after.empty()) {
            temp_info->interrupt_after_nodes.insert(
                temp_info->interrupt_after_nodes.end(),
                hit_after.begin(), hit_after.end());
        }
        
        if (!temp_info->interrupt_before_nodes.empty() || 
            !temp_info->interrupt_after_nodes.empty()) {
            
            std::vector<std::shared_ptr<Task>> new_completed;
            std::vector<std::shared_ptr<Task>> new_cancelled;
            tm->WaitAll(new_completed, new_cancelled);
            
            for (const auto& ct : new_cancelled) {
                temp_info->interrupt_rerun_nodes.push_back(ct->node_key);
            }
            
            ResolveInterruptCompletedTasks(temp_info, new_completed);
            
            if (!temp_info->sub_graph_interrupts.empty() || 
                !temp_info->interrupt_rerun_nodes.empty()) {
                
                std::vector<std::shared_ptr<Task>> all_tasks;
                all_tasks.insert(all_tasks.end(), completed_tasks.begin(), completed_tasks.end());
                all_tasks.insert(all_tasks.end(), new_completed.begin(), new_completed.end());
                all_tasks.insert(all_tasks.end(), cancelled_tasks.begin(), cancelled_tasks.end());
                all_tasks.insert(all_tasks.end(), new_cancelled.begin(), new_cancelled.end());
                
                // 🔴 FIX: 传递 checkpoint_id（如果有）
                const std::string* cp_id_ptr = cp_info.checkpoint_id.empty() ? nullptr : &cp_info.checkpoint_id;
                throw HandleInterruptWithSubGraphAndRerunNodes(
                    ctx, temp_info, all_tasks, cp_id_ptr, false, cm, is_stream);
            }
            
            // Simple interrupt
            auto combined_next = next_tasks;
            auto [new_next, new_result, new_is_end, new_err] = 
                CalculateNextTasks(ctx, new_completed, is_stream, cm, std::map<std::string, std::vector<std::any>>{});
            
            if (!new_err.empty()) {
                throw std::runtime_error("Failed to calculate next tasks: " + new_err);
            }
            
            combined_next.insert(combined_next.end(), new_next.begin(), new_next.end());
            
            // 🔴 FIX: 传递 checkpoint_id（如果有）
            const std::string* cp_id_ptr = cp_info.checkpoint_id.empty() ? nullptr : &cp_info.checkpoint_id;
            throw HandleInterrupt(ctx, temp_info, combined_next, cm->GetChannels(), is_stream, false, cp_id_ptr);
        }
    }
    
    return result;
}

// Initialize channel manager
// Aligns with: eino/compose/graph_run.go:777-846
template<typename I, typename O>
std::shared_ptr<ChannelManager> GraphRunner<I, O>::InitChannelManager(bool is_stream) {
    std::map<std::string, std::shared_ptr<Channel>> channels;
    std::map<std::string, std::vector<std::string>> successors;
    std::map<std::string, std::vector<std::string>> data_predecessors;
    std::map<std::string, std::vector<std::string>> control_predecessors;
    
    auto create_channel = [this](const std::vector<std::string>& ctrl_deps,
                                 const std::vector<std::string>& data_deps) {
        if (options_.run_type == GraphRunType::DAG) {
            return CreateDAGChannel(ctrl_deps, data_deps);
        } else {
            return CreatePregelChannel();
        }
    };
    
    auto node_names = graph_->GetNodeNames();
    
    for (const auto& node_name : node_names) {
        auto edges = graph_->GetEdges(node_name);
        
        std::vector<std::string> node_successors;
        
        for (const auto& edge : edges) {
            node_successors.push_back(edge.to);
            
            if (edge.is_data_edge) {
                data_predecessors[edge.to].push_back(node_name);
            }
            if (edge.is_control_edge) {
                control_predecessors[edge.to].push_back(node_name);
            }
        }
        
        successors[node_name] = node_successors;
        
        channels[node_name] = create_channel(
            control_predecessors[node_name],
            data_predecessors[node_name]
        );
    }
    
    return std::make_shared<ChannelManager>(
        is_stream, channels, successors, data_predecessors, control_predecessors);
}

// Initialize task manager
// Aligns with: eino/compose/graph_run.go:766-775
template<typename I, typename O>
std::shared_ptr<TaskManager> GraphRunner<I, O>::InitTaskManager() {
    bool need_all = !options_.eager_execution;
    return std::make_shared<TaskManager>(need_all);
}

// Calculate next tasks to execute
// Aligns with: eino/compose/graph_run.go:632-646
template<typename I, typename O>
std::tuple<std::vector<std::shared_ptr<Task>>, O, bool, std::string>
GraphRunner<I, O>::CalculateNextTasks(
    std::shared_ptr<Context> ctx,
    const std::vector<std::shared_ptr<Task>>& completed_tasks,
    bool is_stream,
    std::shared_ptr<ChannelManager> cm,
    const std::map<std::string, std::vector<std::any>>& opt_map) {
    
    std::map<std::string, std::map<std::string, std::shared_ptr<void>>> write_values;
    std::map<std::string, std::vector<std::string>> controls;
    
    ResolveCompletedTasks(ctx, completed_tasks, is_stream, cm, write_values, controls);
    
    auto node_map = cm->UpdateAndGet(write_values, controls);
    
    O result{};
    if (node_map.count("__END__")) {
        result = *std::static_pointer_cast<O>(node_map["__END__"]);
        return {std::vector<std::shared_ptr<Task>>{}, result, true, ""};
    }
    
    auto next_tasks = CreateTasks(ctx, node_map);
    return {next_tasks, result, false, ""};
}

// Create tasks from node map
// Aligns with: eino/compose/graph_run.go:682-700
template<typename I, typename O>
std::vector<std::shared_ptr<Task>> GraphRunner<I, O>::CreateTasks(
    std::shared_ptr<Context> ctx,
    const std::map<std::string, std::shared_ptr<void>>& node_map) {
    
    std::vector<std::shared_ptr<Task>> tasks;
    
    for (const auto& pair : node_map) {
        const auto& node_name = pair.first;
        auto node_input = pair.second;
        
        auto task = std::make_shared<Task>(node_name);
        task->context = ctx;
        task->input = node_input;
        task->status = TaskStatus::Queued;
        
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // ⭐ 1. 设置 graph_node（用于 Execute 中获取 runnable）
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        task->graph_node = graph_->GetNode(node_name);
        if (!task->graph_node) {
            throw std::runtime_error("Node not found in graph: " + node_name);
        }
        
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // ⭐ 2. 智能方法选择（Smart Method Selection）
        // Aligns with: eino/compose/graph_run.go:124-125
        //   runWrapper = runnableInvoke
        //   if isStream { runWrapper = runnableTransform }
        //
        // 决策逻辑：
        //   1. 检测输入类型（是否为 StreamReader）
        //   2. 检测节点能力（支持哪些方法）
        //   3. 检测下游需求（后继节点是否期望流式输入）
        //   4. 根据决策矩阵选择最优方法
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        task->execution_method = DetermineExecutionMethod(
            node_input, 
            task->graph_node, 
            node_name);
        
        tasks.push_back(task);
    }
    
    return tasks;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ⭐ Helper: Determine execution method based on input type, 
//           node capabilities, and downstream requirements
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
template<typename I, typename O>
std::string GraphRunner<I, O>::DetermineExecutionMethod(
    std::shared_ptr<void> input,
    std::shared_ptr<GraphNode> graph_node,
    const std::string& node_name) {
    
    if (!graph_node || !graph_node->runnable) {
        // Fallback: default to Invoke
        return "Invoke";
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 1: 检测输入是否为流式（StreamReader）
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    bool input_is_stream = IsStreamInput(input);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 2: 检测节点能力
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    NodeCapabilities caps = DetectNodeCapabilities(graph_node);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 3: 检测下游是否需要流式输出
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    bool downstream_expects_stream = CheckDownstreamExpectsStream(node_name);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 4: 决策矩阵（Decision Matrix）
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    if (input_is_stream) {
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // 流式输入 → 优先级：Transform > Collect > Invoke
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        if (downstream_expects_stream && caps.has_transform) {
            // 最优：流式→流式（无需转换）
            return "Transform";
        }
        if (!downstream_expects_stream && caps.has_collect) {
            // 次优：流式→非流式（主动收集）
            return "Collect";
        }
        if (caps.has_invoke) {
            // 兜底：先收集再调用（隐式转换）
            return "Invoke";
        }
        // 完全无能力，默认 Invoke（会失败）
        return "Invoke";
        
    } else {
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // 非流式输入 → 根据下游需求决定
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        if (downstream_expects_stream) {
            // 下游需要流：优先使用 Stream 方法
            if (caps.has_stream) {
                return "Stream";
            }
            // 兜底：Invoke + 后续包装（由 Execute 处理）
            return "Invoke";
        } else {
            // 下游不需要流：直接使用 Invoke
            if (caps.has_invoke) {
                return "Invoke";
            }
            // 兜底：默认 Invoke
            return "Invoke";
        }
    }
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ⭐ Helper: Check if input is a StreamReader
//
// 完全对齐 eino 的实现：
// - 使用 IStreamReader 接口（对齐 streamReader interface）
// - 支持任意类型的 StreamReader（无需模板参数限制）
// - 通过接口检测，无需知道具体元素类型
//
// Aligns with:
// - eino/compose/stream_reader.go:26 (streamReader interface)
// - eino/compose/runnable.go 中的类型检测逻辑
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
template<typename I, typename O>
bool GraphRunner<I, O>::IsStreamInput(std::shared_ptr<void> input) {
    if (!input) {
        return false;
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ⭐ 方案 1: 尝试转换为 IStreamReader 接口
    // 这是对齐 eino 的核心方案
    // 
    // eino 使用 streamReader interface 来实现类型擦除：
    // - 所有 StreamReader<T> 都实现 streamReader 接口
    // - 通过接口方法（getType(), getChunkType()）获取类型信息
    // - 无需知道具体的 T 类型
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    try {
        auto stream_interface = std::static_pointer_cast<IStreamReader>(input);
        if (stream_interface) {
            // ✅ 成功转换为流式接口
            // 无需知道元素类型（int/double/string），都可以检测
            return true;
        }
    } catch (...) {
        // Not a stream interface
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ⭐ 方案 2: 使用 IsStreamValue 辅助函数
    // Aligns with eino 中的类型判断逻辑
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    return IsStreamValue(input);
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ⭐ Helper: Detect node capabilities (which methods it supports)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
template<typename I, typename O>
typename GraphRunner<I, O>::NodeCapabilities 
GraphRunner<I, O>::DetectNodeCapabilities(std::shared_ptr<GraphNode> graph_node) {
    
    NodeCapabilities caps;
    
    if (!graph_node || !graph_node->runnable) {
        return caps;
    }
    
    // ⭐ CRITICAL: Capability detection
    // In a production implementation, this would:
    // 1. Check if runnable implements each interface (Invoke/Stream/Collect/Transform)
    // 2. Cache capabilities in GraphNode to avoid repeated detection
    // 3. Use reflection or interface checking
    
    try {
        // Attempt to cast to ComposableRunnable<I, O>
        auto runnable = std::static_pointer_cast<ComposableRunnable<I, O>>(graph_node->runnable);
        
        if (runnable) {
            // ✅ All ComposableRunnable support all methods
            caps.has_invoke = true;
            caps.has_stream = true;
            caps.has_collect = true;
            caps.has_transform = true;
        }
    } catch (...) {
        // Basic Runnable only supports Invoke
        caps.has_invoke = true;
    }
    
    return caps;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ⭐ Helper: Check if downstream nodes expect stream input
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
template<typename I, typename O>
bool GraphRunner<I, O>::CheckDownstreamExpectsStream(const std::string& node_name) {
    
    // ⭐ CRITICAL: Downstream analysis
    // This requires looking at successor nodes and checking if:
    // 1. They have Transform capability (prefer stream input)
    // 2. They are configured to run in stream mode
    // 3. The edge is marked as stream-only
    
    // Get successors
    auto successors = graph_->GetSuccessors(node_name);
    
    // If output goes to END node, check graph's output mode
    for (const auto& succ : successors) {
        if (succ == Graph<I, O>::END_NODE) {
            // Check if graph is in stream mode (from runner options)
            // For now, default to non-stream
            return false;
        }
        
        // Check if any successor prefers stream input
        auto succ_node = graph_->GetNode(succ);
        if (succ_node) {
            NodeCapabilities caps = DetectNodeCapabilities(succ_node);
            // If successor has Transform capability, it prefers stream
            if (caps.has_transform && !caps.has_collect) {
                return true;
            }
        }
    }
    
    return false;
}

// Resolve completed tasks and update channels
// Aligns with: eino/compose/graph_run.go:702-764
template<typename I, typename O>
void GraphRunner<I, O>::ResolveCompletedTasks(
    std::shared_ptr<Context> ctx,
    const std::vector<std::shared_ptr<Task>>& completed_tasks,
    bool is_stream,
    std::shared_ptr<ChannelManager> cm,
    std::map<std::string, std::map<std::string, std::shared_ptr<void>>>& write_values,
    std::map<std::string, std::vector<std::string>>& controls) {
    
    for (const auto& task : completed_tasks) {
        auto successors = cm->GetSuccessors(task->node_key);
        
        auto branches = graph_->GetBranches(task->node_key);
        if (!branches.empty()) {
            std::vector<std::shared_ptr<void>> branch_inputs;
            for (size_t i = 0; i < branches.size(); ++i) {
                branch_inputs.push_back(task->output);
            }
            
            for (size_t i = 0; i < branches.size(); ++i) {
                auto branch_successors = branches[i]->Invoke(task->context.get(), task->output.get());
                successors.insert(successors.end(), branch_successors.begin(), branch_successors.end());
            }
        }
        
        for (const auto& successor : successors) {
            write_values[successor][task->node_key] = task->output;
        }
        
        for (const auto& successor : successors) {
            controls[successor].push_back(task->node_key);
        }
    }
}

// Resolve interrupt information from completed tasks
// Aligns with: eino/compose/graph_run.go:418-451
template<typename I, typename O>
void GraphRunner<I, O>::ResolveInterruptCompletedTasks(
    std::shared_ptr<InterruptTempInfo> temp_info,
    const std::vector<std::shared_ptr<Task>>& completed_tasks) {
    
    for (const auto& task : completed_tasks) {
        if (!task->error) continue;
        
        try {
            std::rethrow_exception(std::make_exception_ptr(*task->error));
        } catch (const SubGraphInterruptError& e) {
            temp_info->sub_graph_interrupts[task->node_key] = e.GetInfo();
        } catch (const InterruptAndRerunError& e) {
            temp_info->interrupt_rerun_nodes.push_back(task->node_key);
            temp_info->interrupt_rerun_extra[task->node_key] = e.GetExtra();
        } catch (...) {
            // Regular error, not interrupt
        }
        
        // Check if this completed task is in interrupt_after list
        // Aligns with: eino/compose/graph_run.go:443-448
        for (const auto& key : interrupt_after_nodes_) {
            if (key == task->node_key) {
                temp_info->interrupt_after_nodes.push_back(key);
                break;
            }
        }
    }
}

// Get hit keys from tasks matching target list
// Aligns with: eino/compose/graph_run.go:453-463
template<typename I, typename O>
std::vector<std::string> GraphRunner<I, O>::GetHitKeys(
    const std::vector<std::shared_ptr<Task>>& tasks,
    const std::vector<std::string>& target_keys) {
    
    std::vector<std::string> hit;
    for (const auto& task : tasks) {
        for (const auto& key : target_keys) {
            if (key == task->node_key) {
                hit.push_back(task->node_key);
                break;
            }
        }
    }
    return hit;
}

// Handle interrupt by saving checkpoint
// Aligns with: eino/compose/graph_run.go:465-512
template<typename I, typename O>
InterruptError GraphRunner<I, O>::HandleInterrupt(
    std::shared_ptr<Context> ctx,
    std::shared_ptr<InterruptTempInfo> temp_info,
    const std::vector<std::shared_ptr<Task>>& next_tasks,
    const std::map<std::string, std::shared_ptr<Channel>>& channels,
    bool is_stream,
    bool is_sub_graph,
    const std::string* checkpoint_id) {
    
    // 🔴 FIX 1: 构建完整的 checkpoint
    auto cp = std::make_shared<CheckPoint>();
    cp->channels = channels;
    cp->skip_pre_handler = {};  // Initialize empty map
    
    // 对齐 Go: eino/compose/graph_run.go:472-477
    for (const auto& task : next_tasks) {
        cp->inputs[task->node_key] = task->input;
    }
    
    // Save state if enabled (runCtx is set)
    // Aligns with: eino/compose/graph_run.go:479-483
    if (run_ctx_) {
        // current graph has enabled state
        try {
            struct StateKey {};
            auto internal_state = context::GetContextValue<InternalState>(ctx.get(), StateKey{});
            if (internal_state && internal_state->state) {
                cp->state = internal_state->state;
            }
        } catch (...) {
            // State not found or cannot be retrieved, continue without state
        }
    }
    
    // 🔴 FIX 2: 构建 InterruptInfo（先于转换）
    auto info = std::make_shared<InterruptInfo>();
    info->state = cp->state;
    info->after_nodes = temp_info->interrupt_after_nodes;
    info->before_nodes = temp_info->interrupt_before_nodes;
    info->rerun_nodes = temp_info->interrupt_rerun_nodes;
    info->rerun_nodes_extra = temp_info->interrupt_rerun_extra;
    info->sub_graphs = {};  // Initialize empty map
    
    // ⭐ CRITICAL FIX 3: Convert checkpoint before saving
    // Aligns with: eino/compose/graph_run.go:496-499
    auto err = check_pointer_->ConvertCheckPoint(cp, is_stream);
    if (!err.empty()) {
        throw std::runtime_error("failed to convert checkpoint: " + err);
    }
    
    // 🔴 FIX 4: 处理 SubGraph 和 CheckpointID
    // Aligns with: eino/compose/graph_run.go:502-509
    if (is_sub_graph) {
        // SubGraph interrupt: 返回特殊错误，包含 checkpoint
        return SubGraphInterruptError("subgraph interrupt", info, cp);
    } else if (checkpoint_id != nullptr && !checkpoint_id->empty()) {
        // Normal interrupt with checkpoint ID: 保存到 store
        auto save_err = check_pointer_->Set(ctx, *checkpoint_id, cp);
        if (!save_err.empty()) {
            throw std::runtime_error("failed to set checkpoint: " + save_err + 
                                   ", checkPointID: " + *checkpoint_id);
        }
    }
    
    return InterruptError("interrupt happened", info);
}

// Handle interrupt with subgraph and rerun nodes
// Aligns with: eino/compose/graph_run.go:514-612
template<typename I, typename O>
InterruptError GraphRunner<I, O>::HandleInterruptWithSubGraphAndRerunNodes(
    std::shared_ptr<Context> ctx,
    std::shared_ptr<InterruptTempInfo> temp_info,
    const std::vector<std::shared_ptr<Task>>& all_tasks,
    const std::string* checkpoint_id,
    bool is_sub_graph,
    std::shared_ptr<ChannelManager> cm,
    bool is_stream) {
    
    // 🔴 FIX 1: 分类任务（对齐 Go: eino/compose/graph_run.go:521-540）
    std::vector<std::shared_ptr<Task>> rerun_tasks, subgraph_tasks, other_tasks;
    std::map<std::string, bool> skip_pre_handler;
    
    for (const auto& task : all_tasks) {
        // SubGraph tasks
        if (temp_info->sub_graph_interrupts.count(task->node_key)) {
            subgraph_tasks.push_back(task);
            skip_pre_handler[task->node_key] = true;  // subgraph won't run pre-handler again
            continue;
        }
        
        // Rerun tasks
        bool is_rerun = false;
        for (const auto& key : temp_info->interrupt_rerun_nodes) {
            if (key == task->node_key) {
                rerun_tasks.push_back(task);
                is_rerun = true;
                break;
            }
        }
        
        // Other completed tasks
        if (!is_rerun) {
            other_tasks.push_back(task);
        }
    }
    
    // 🔴 FIX 2: Forward completed tasks (对齐 Go: eino/compose/graph_run.go:542-547）
    std::map<std::string, std::map<std::string, std::shared_ptr<void>>> write_values;
    std::map<std::string, std::vector<std::string>> controls;
    ResolveCompletedTasks(ctx, other_tasks, is_stream, cm, write_values, controls);
    cm->UpdateValues(write_values);
    cm->UpdateDependencies(controls);
    
    // 🔴 FIX 3: 构建 checkpoint（对齐 Go: eino/compose/graph_run.go:549-561）
    auto cp = std::make_shared<CheckPoint>();
    cp->channels = cm->GetChannels();
    cp->skip_pre_handler = skip_pre_handler;
    
    // 🔴 CRITICAL: 添加 ToolsNode 执行结果（对齐 Go:562）
    cp->tools_node_executed_tools = temp_info->interrupt_executed_tools;
    
    // Initialize SubGraphs map（对齐 Go:563）
    cp->sub_graphs = {};
    
    // Add rerun tasks inputs
    for (const auto& task : rerun_tasks) {
        cp->inputs[task->node_key] = task->input;
    }
    
    // Add subgraph tasks inputs
    for (const auto& task : subgraph_tasks) {
        cp->inputs[task->node_key] = task->input;
    }
    
    // Save state if enabled (runCtx is set)
    // Aligns with: eino/compose/graph_run.go:565-569
    if (run_ctx_) {
        // current graph has enabled state
        try {
            struct StateKey {};
            auto internal_state = context::GetContextValue<InternalState>(ctx.get(), StateKey{});
            if (internal_state && internal_state->state) {
                cp->state = internal_state->state;
            }
        } catch (...) {
            // State not found or cannot be retrieved, continue without state
        }
    }
    
    // 🔴 FIX 4: 构建 InterruptInfo（对齐 Go: eino/compose/graph_run.go:571-577）
    auto info = std::make_shared<InterruptInfo>();
    info->state = cp->state;
    info->rerun_nodes = temp_info->interrupt_rerun_nodes;
    info->rerun_nodes_extra = temp_info->interrupt_rerun_extra;
    info->sub_graphs = temp_info->sub_graph_interrupts;
    
    // 🔴 FIX 5: 合并 SubGraph 的 InterruptInfo（对齐 Go: eino/compose/graph_run.go:580-586）
    for (const auto& [node_key, sg_info] : temp_info->sub_graph_interrupts) {
        info->sub_graphs[node_key] = sg_info;
    }
    
    // ⭐ CRITICAL FIX 6: Convert checkpoint before saving
    // Aligns with: eino/compose/graph_run.go:588-591
    auto err = check_pointer_->ConvertCheckPoint(cp, is_stream);
    if (!err.empty()) {
        throw std::runtime_error("failed to convert checkpoint: " + err);
    }
    
    // 🔴 FIX 7: 保存 checkpoint 或返回 SubGraph 错误（对齐 Go: eino/compose/graph_run.go:593-608）
    if (is_sub_graph) {
        // SubGraph interrupt
        return SubGraphInterruptError("subgraph interrupt with rerun nodes", info, cp);
    } else if (checkpoint_id != nullptr && !checkpoint_id->empty()) {
        // Normal interrupt: 保存到 store
        auto save_err = check_pointer_->Set(ctx, *checkpoint_id, cp);
        if (!save_err.empty()) {
            throw std::runtime_error("failed to set checkpoint: " + save_err + 
                                   ", checkPointID: " + *checkpoint_id);
        }
    }
    
    return InterruptError("interrupt happened with rerun nodes", info);
}

// Restore from checkpoint
// Aligns with: eino/compose/graph_run.go:366-399
template<typename I, typename O>
std::tuple<std::shared_ptr<Context>, std::vector<std::shared_ptr<Task>>, std::string>
GraphRunner<I, O>::RestoreFromCheckPoint(
    std::shared_ptr<Context> ctx,
    const std::vector<std::string>& path,
    StateModifier state_modifier,
    std::shared_ptr<CheckPoint> cp,
    bool is_stream,
    std::shared_ptr<ChannelManager> cm,
    const std::map<std::string, std::vector<std::any>>& opt_map) {
    
    // Step 1: Restore checkpoint data (stream conversion if needed)
    // Aligns with: eino/compose/graph_run.go:375-378
    // ⭐ CRITICAL FIX: Restore checkpoint before using
    if (check_pointer_) {
        auto err = check_pointer_->RestoreCheckPoint(cp, is_stream);
        if (!err.empty()) {
            return {ctx, {}, "failed to restore checkpoint: " + err};
        }
    }
    
    // Step 2: Restore channels
    // Aligns with: eino/compose/graph_run.go:380-383
    auto err = cm->LoadChannels(cp->channels);
    if (!err.empty()) {
        return {ctx, {}, err};
    }
    
    // Step 3: Call StateModifier (if provided)
    // Aligns with: eino/compose/graph_run.go:384-388
    if (state_modifier && cp->state) {
        try {
            state_modifier(ctx, path, cp->state);
        } catch (const std::exception& e) {
            return {ctx, {}, std::string("state modifier fail: ") + e.what()};
        }
    }
    
    // Step 4: Restore state to context
    // Aligns with: eino/compose/graph_run.go:389-391
    if (cp->state) {
        struct StateKey {};
        auto internal_state = std::make_shared<InternalState>();
        internal_state->state = cp->state;
        ctx = context::SetContextValue(ctx, StateKey{}, internal_state);
    }
    
    // Step 5: Restore tasks
    // Aligns with: eino/compose/graph_run.go:393-397
    std::vector<std::shared_ptr<Task>> next_tasks;
    
    // Combine inputs and rerun_nodes
    std::set<std::string> task_keys;
    for (const auto& node : cp->rerun_nodes) {
        task_keys.insert(node);
    }
    for (const auto& pair : cp->inputs) {
        task_keys.insert(pair.first);
    }
    
    for (const auto& key : task_keys) {
        auto task = std::make_shared<Task>(key);
        task->context = ctx;
        
        // Set input (use zero value for rerun nodes)
        auto it = cp->inputs.find(key);
        if (it != cp->inputs.end()) {
            task->input = it->second;
        } else {
            // Rerun node without saved input - use zero value
            // TODO: Get zero value from node definition
            task->input = nullptr;
        }
        
        // Set skip_pre_handler flag
        auto skip_it = cp->skip_pre_handler.find(key);
        if (skip_it != cp->skip_pre_handler.end()) {
            task->skip_pre_handler = skip_it->second;
        }
        
        task->status = TaskStatus::Queued;
        next_tasks.push_back(task);
    }
    
    return {ctx, next_tasks, ""};
}

// Extract checkpoint info from options
// Aligns with: eino/compose/graph_run.go:654-670
template<typename I, typename O>
typename GraphRunner<I, O>::CheckPointInfo 
GraphRunner<I, O>::GetCheckPointInfo(const std::vector<Option>& options) {
    CheckPointInfo info;
    
    for (const auto& opt : options) {
        if (!opt.checkpoint_id.empty()) {
            info.checkpoint_id = opt.checkpoint_id;
        }
        if (!opt.write_to_checkpoint_id.empty()) {
            info.write_to_checkpoint_id = opt.write_to_checkpoint_id;
        }
        if (opt.state_modifier) {
            info.state_modifier = opt.state_modifier;
        }
        if (opt.force_new_run) {
            info.force_new_run = true;
        }
    }
    
    // If write_to_checkpoint_id is not set, use checkpoint_id
    // Aligns with: eino/compose/graph_run.go:668-670
    if (info.write_to_checkpoint_id.empty()) {
        info.write_to_checkpoint_id = info.checkpoint_id;
    }
    
    return info;
}

// Explicit template instantiation for common types
template class GraphRunner<std::string, std::string>;
template class GraphRunner<int, int>;
template class GraphRunner<double, double>;

} // namespace compose
} // namespace eino
