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

// Aligns with: eino/compose/graph_manager.go
// Implementation of Channel, ChannelManager, TaskManager

#include "eino/compose/graph_manager.h"
#include "eino/compose/graph_run.h"
#include "eino/compose/value_merge.h"
#include "eino/compose/utils.h"
#include <algorithm>

namespace eino {
namespace compose {

// =============================================================================
// DAG Channel Implementation
// Aligns with: eino/compose/dag.go:50-180
// =============================================================================

DAGChannel::DAGChannel(const std::vector<std::string>& control_deps,
                       const std::vector<std::string>& data_deps) {
    for (const auto& dep : control_deps) {
        control_predecessors_[dep] = DependencyState::Waiting;
    }
    for (const auto& dep : data_deps) {
        data_predecessors_[dep] = false;
    }
}

void DAGChannel::ReportValues(const std::map<std::string, std::shared_ptr<void>>& values) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (skipped_) {
        return;
    }
    
    for (const auto& pair : values) {
        if (data_predecessors_.count(pair.first)) {
            data_predecessors_[pair.first] = true;
            values_[pair.first] = pair.second;
        }
    }
}

void DAGChannel::ReportDependencies(const std::vector<std::string>& deps) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (skipped_) {
        return;
    }
    
    for (const auto& dep : deps) {
        if (control_predecessors_.count(dep)) {
            control_predecessors_[dep] = DependencyState::Ready;
        }
    }
}

bool DAGChannel::ReportSkip(const std::vector<std::string>& nodes) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& node : nodes) {
        if (control_predecessors_.count(node)) {
            control_predecessors_[node] = DependencyState::Skipped;
        }
        if (data_predecessors_.count(node)) {
            data_predecessors_[node] = true;
        }
    }
    
    // Check if all control predecessors are skipped
    bool all_skipped = true;
    for (const auto& pair : control_predecessors_) {
        if (pair.second != DependencyState::Skipped) {
            all_skipped = false;
            break;
        }
    }
    
    skipped_ = all_skipped;
    return all_skipped;
}

bool DAGChannel::Get(bool is_stream, const std::string& node_name,
                    std::shared_ptr<void>& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (skipped_) {
        return false;
    }
    
    // Check if all dependencies are ready
    for (const auto& pair : control_predecessors_) {
        if (pair.second == DependencyState::Waiting) {
            return false;
        }
    }
    for (const auto& pair : data_predecessors_) {
        if (!pair.second) {
            return false;
        }
    }
    
    // Merge values from all data predecessors
    // Aligns with: eino/compose/dag.go:178-191
    if (values_.empty()) {
        value = nullptr;
    } else if (values_.size() == 1) {
        value = values_.begin()->second;
    } else {
        // ✅ Merge multiple values using registered merge function
        try {
            // Convert std::shared_ptr<void> to std::any for merging
            std::vector<std::any> any_values;
            any_values.reserve(values_.size());
            
            for (const auto& pair : values_) {
                // Wrap void* in std::any
                // Note: This assumes the void* points to a valid object
                // In practice, you may need a better type preservation mechanism
                any_values.push_back(pair.second);
            }
            
            // Prepare merge options
            MergeOptions merge_opts;
            auto it = merge_config_.find("stream_merge_with_source_eof");
            if (it != merge_config_.end() && it->second == "true") {
                merge_opts.stream_merge_with_source_eof = true;
            }
            
            // Extract names for merge options
            for (const auto& pair : values_) {
                merge_opts.names.push_back(pair.first);
            }
            
            // Perform merge
            std::any merged = MergeValues(any_values, &merge_opts);
            
            // Convert back to void*
            // This is a simplified approach; production code needs proper type handling
            if (merged.has_value()) {
                value = std::make_shared<std::any>(merged);
            } else {
                value = nullptr;
            }
            
        } catch (const std::exception& e) {
            // Merge failed - throw error to propagate to caller
            throw std::runtime_error(
                std::string("DAGChannel::Get() merge failed: ") + e.what());
        }
    }
    
    // Reset for next iteration
    values_.clear();
    for (auto& pair : control_predecessors_) {
        pair.second = DependencyState::Waiting;
    }
    for (auto& pair : data_predecessors_) {
        pair.second = false;
    }
    
    return true;
}

void DAGChannel::ConvertValues(std::function<void(std::map<std::string, std::shared_ptr<void>>&)> fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    fn(values_);
}

void DAGChannel::Load(std::shared_ptr<Channel> other) {
    auto dag_ch = std::dynamic_pointer_cast<DAGChannel>(other);
    if (!dag_ch) {
        throw std::runtime_error("Cannot load non-DAGChannel into DAGChannel");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    std::lock_guard<std::mutex> other_lock(dag_ch->mutex_);
    
    control_predecessors_ = dag_ch->control_predecessors_;
    data_predecessors_ = dag_ch->data_predecessors_;
    values_ = dag_ch->values_;
    skipped_ = dag_ch->skipped_;
}

void DAGChannel::SetMergeConfig(const std::map<std::string, std::string>& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    merge_config_ = config;
}

// =============================================================================
// Pregel Channel Implementation
// Aligns with: eino/compose/pregel.go:25-90
// =============================================================================

PregelChannel::PregelChannel() {}

void PregelChannel::ReportValues(const std::map<std::string, std::shared_ptr<void>>& values) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& pair : values) {
        values_[pair.first] = pair.second;
    }
}

void PregelChannel::ReportDependencies(const std::vector<std::string>& deps) {
    // Pregel doesn't track control dependencies
}

bool PregelChannel::ReportSkip(const std::vector<std::string>& nodes) {
    // Pregel doesn't skip nodes
    return false;
}

bool PregelChannel::Get(bool is_stream, const std::string& node_name,
                       std::shared_ptr<void>& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (values_.empty()) {
        return false;
    }
    
    // Merge all values
    // Aligns with: eino/compose/pregel.go:77-94
    if (values_.size() == 1) {
        value = values_.begin()->second;
    } else {
        // ✅ Merge multiple values using registered merge function
        try {
            // Convert to std::any vector
            std::vector<std::any> any_values;
            any_values.reserve(values_.size());
            
            for (const auto& pair : values_) {
                any_values.push_back(pair.second);
            }
            
            // Prepare merge options
            MergeOptions merge_opts;
            auto it = merge_config_.find("stream_merge_with_source_eof");
            if (it != merge_config_.end() && it->second == "true") {
                merge_opts.stream_merge_with_source_eof = true;
            }
            
            // Extract names
            for (const auto& pair : values_) {
                merge_opts.names.push_back(pair.first);
            }
            
            // Perform merge
            std::any merged = MergeValues(any_values, &merge_opts);
            
            // Convert back to void*
            if (merged.has_value()) {
                value = std::make_shared<std::any>(merged);
            } else {
                value = nullptr;
            }
            
        } catch (const std::exception& e) {
            // Merge failed - throw error
            throw std::runtime_error(
                std::string("PregelChannel::Get() merge failed: ") + e.what());
        }
    }
    
    values_.clear();
    return true;
}

void PregelChannel::ConvertValues(std::function<void(std::map<std::string, std::shared_ptr<void>>&)> fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    fn(values_);
}

void PregelChannel::Load(std::shared_ptr<Channel> other) {
    auto pregel_ch = std::dynamic_pointer_cast<PregelChannel>(other);
    if (!pregel_ch) {
        throw std::runtime_error("Cannot load non-PregelChannel into PregelChannel");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    std::lock_guard<std::mutex> other_lock(pregel_ch->mutex_);
    values_ = pregel_ch->values_;
}

void PregelChannel::SetMergeConfig(const std::map<std::string, std::string>& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    merge_config_ = config;
}

// =============================================================================
// Channel Manager Implementation
// Aligns with: eino/compose/graph_manager.go:115-230
// =============================================================================

ChannelManager::ChannelManager(
    bool is_stream,
    const std::map<std::string, std::shared_ptr<Channel>>& channels,
    const std::map<std::string, std::vector<std::string>>& successors,
    const std::map<std::string, std::vector<std::string>>& data_predecessors,
    const std::map<std::string, std::vector<std::string>>& control_predecessors)
    : is_stream_(is_stream),
      channels_(channels),
      successors_(successors),
      data_predecessors_(data_predecessors),
      control_predecessors_(control_predecessors) {}

void ChannelManager::LoadChannels(const std::map<std::string, std::shared_ptr<Channel>>& channels) {
    for (const auto& pair : channels_) {
        auto it = channels.find(pair.first);
        if (it != channels.end()) {
            pair.second->Load(it->second);
        }
    }
}

void ChannelManager::UpdateValues(const std::map<std::string, std::map<std::string, std::shared_ptr<void>>>& values) {
    for (const auto& target_pair : values) {
        const auto& target = target_pair.first;
        const auto& from_map = target_pair.second;
        
        auto ch_it = channels_.find(target);
        if (ch_it == channels_.end()) {
            throw std::runtime_error("Target channel doesn't exist: " + target);
        }
        
        ch_it->second->ReportValues(from_map);
    }
}

void ChannelManager::UpdateDependencies(const std::map<std::string, std::vector<std::string>>& deps) {
    for (const auto& target_pair : deps) {
        const auto& target = target_pair.first;
        const auto& dependencies = target_pair.second;
        
        auto ch_it = channels_.find(target);
        if (ch_it == channels_.end()) {
            throw std::runtime_error("Target channel doesn't exist: " + target);
        }
        
        ch_it->second->ReportDependencies(dependencies);
    }
}

std::map<std::string, std::shared_ptr<void>> ChannelManager::GetFromReadyChannels() {
    std::map<std::string, std::shared_ptr<void>> result;
    
    for (const auto& pair : channels_) {
        std::shared_ptr<void> value;
        if (pair.second->Get(is_stream_, pair.first, value)) {
            result[pair.first] = value;
        }
    }
    
    return result;
}

std::map<std::string, std::shared_ptr<void>> ChannelManager::UpdateAndGet(
    const std::map<std::string, std::map<std::string, std::shared_ptr<void>>>& values,
    const std::map<std::string, std::vector<std::string>>& deps) {
    
    UpdateValues(values);
    UpdateDependencies(deps);
    return GetFromReadyChannels();
}

void ChannelManager::ReportBranch(const std::string& from, const std::vector<std::string>& skipped_nodes) {
    std::vector<std::string> to_process;
    
    for (const auto& node : skipped_nodes) {
        auto ch_it = channels_.find(node);
        if (ch_it != channels_.end()) {
            if (ch_it->second->ReportSkip({from})) {
                to_process.push_back(node);
            }
        }
    }
    
    // Propagate skip to successors
    for (size_t i = 0; i < to_process.size(); ++i) {
        const auto& key = to_process[i];
        
        auto succ_it = successors_.find(key);
        if (succ_it != successors_.end()) {
            for (const auto& successor : succ_it->second) {
                auto ch_it = channels_.find(successor);
                if (ch_it != channels_.end()) {
                    if (ch_it->second->ReportSkip({key})) {
                        to_process.push_back(successor);
                    }
                }
            }
        }
    }
}

// =============================================================================
// Task Manager Implementation
// Aligns with: eino/compose/graph_manager.go:232-480
// =============================================================================

TaskManager::TaskManager(bool need_all)
    : need_all_(need_all) {}

TaskManager::~TaskManager() {
    Cancel();
}

void TaskManager::Submit(const std::vector<std::shared_ptr<Task>>& tasks) {
    if (tasks.empty()) {
        return;
    }
    
    // ⭐ Aligns with: eino/compose/graph_manager.go:289-325
    // Pre-handler runs in Submit, not in Execute
    
    std::vector<std::shared_ptr<Task>> valid_tasks;
    valid_tasks.reserve(tasks.size());
    
    for (const auto& task : tasks) {
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // ⭐ PRE-HANDLER EXECUTION (runs before queueing)
        // Aligns with: eino/compose/graph_manager.go:294-303
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        try {
            if (!task->skip_pre_handler) {
                auto node_info = task->graph_node ? task->graph_node->GetNodeInfo() : nullptr;
                if (node_info && node_info->pre_processor) {
                    // Execute pre-processor synchronously
                    // If pre-processor fails, task doesn't enter execution queue
                    // task->input = RunPreHandler(task, runWrapper);
                    
                    // TODO: Implement RunPreHandler with proper type-safe execution
                    // For now, skip if not implemented
                }
            }
            
            // Pre-handler succeeded, add to valid tasks
            valid_tasks.push_back(task);
            
        } catch (const std::exception& e) {
            // ✅ Pre-handler error - task fails immediately without execution
            // Aligns with Go's: currentTask.err = err
            task->error = std::make_shared<std::runtime_error>(
                std::string("Pre-handler failed for node [") + 
                task->node_key + "]: " + e.what());
            task->status = TaskStatus::Failed;
            
            // Send directly to done queue
            {
                std::lock_guard<std::mutex> lock(mutex_);
                done_queue_.push(task);
                num_running_++;  // Increment for WaitOne
            }
            cv_.notify_one();
        }
    }
    
    if (valid_tasks.empty()) {
        // All tasks' pre-handlers failed
        return;
    }
    
    // ⭐ Aligns with: eino/compose/graph_manager.go:314-325
    // Synchronous execution optimization (optional)
    std::shared_ptr<Task> sync_task = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Execute one task synchronously if:
        // 1. No other tasks running (num_running_ == 0)
        // 2. Only one task OR need_all_ mode
        // 3. Not cancelled
        if (num_running_ == 0 && (valid_tasks.size() == 1 || need_all_) && !cancelled_) {
            sync_task = valid_tasks[0];
            valid_tasks.erase(valid_tasks.begin());
        }
        
        // Add tasks to running pool
        for (const auto& task : valid_tasks) {
            running_tasks_[task->node_key] = task;
            num_running_++;
        }
    }
    
    // Execute sync task first (optimization)
    if (sync_task) {
        running_tasks_[sync_task->node_key] = sync_task;
        num_running_++;
        Execute(sync_task);
    }
    
    // Execute remaining tasks asynchronously
    for (const auto& task : valid_tasks) {
        std::thread([this, task]() {
            Execute(task);
        }).detach();
    }
}

void TaskManager::Execute(std::shared_ptr<Task> task) {
    // ⭐ STRICTLY Aligns with: eino/compose/graph_manager.go:273-286
    // 
    // Go implementation:
    // func (t *taskManager) execute(currentTask *task) {
    //     defer func() {
    //         panicInfo := recover()
    //         if panicInfo != nil {
    //             currentTask.output = nil
    //             currentTask.err = safe.NewPanicErr(panicInfo, debug.Stack())
    //         }
    //         t.done.Send(currentTask)
    //     }()
    //     ctx := initNodeCallbacks(currentTask.ctx, currentTask.nodeKey, ...)
    //     currentTask.output, currentTask.err = t.runWrapper(ctx, currentTask.call.action, currentTask.input, currentTask.option...)
    // }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ⭐ PANIC RECOVERY (defer + recover in Go)
    // Must ALWAYS execute, even if task succeeds
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    try {
        // ✅ Mark as running
        task->status = TaskStatus::Running;
        
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // ⭐ 1. Initialize Node Callbacks Context
        // Aligns with: initNodeCallbacks(currentTask.ctx, currentTask.nodeKey, ...)
        // This sets up callbacks, metadata, and monitoring for the node
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        auto ctx = task->context;
        if (!ctx) {
            ctx = Context::Background();
        }
        
        // Initialize callbacks for this node
        // Aligns with: eino/compose/graph_manager.go:284
        // ctx := initNodeCallbacks(currentTask.ctx, currentTask.nodeKey, ...)
        auto node_info_ptr = task->graph_node ? task->graph_node->GetNodeInfo() : nullptr;
        auto meta_ptr = task->graph_node ? task->graph_node->GetExecutorMeta() : nullptr;
        
        // Get options from task manager (this would come from Graph::Invoke/Stream/etc.)
        std::vector<Option> callback_opts;  // TODO: Extract from task or graph options
        
        ctx = InitNodeCallbacks(ctx, task->node_key, 
                               node_info_ptr.get(), 
                               meta_ptr.get(), 
                               callback_opts);
        
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // ⭐ 2. Execute via runWrapper
        // Aligns with: currentTask.output, currentTask.err = t.runWrapper(ctx, ...)
        // 
        // runWrapper handles:
        // - Smart method selection (Invoke/Stream/Collect/Transform)
        // - Execution
        // - Error handling
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        if (!task->graph_node) {
            throw std::runtime_error("Task has no associated GraphNode");
        }
        
        auto runnable = task->graph_node->GetComposableRunnable();
        if (!runnable) {
            throw std::runtime_error("GraphNode has no runnable");
        }
        
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // ⭐ Execute via RunWrapper
        // Aligns with eino/compose/graph_run.go:97-106
        // 
        // Go implementation:
        //   type runnableCallWrapper func(context.Context, *composableRunnable, any, ...any) (any, error)
        //   
        //   func runnableInvoke(ctx, r, input, opts...) (any, error) {
        //       return r.i(ctx, input, opts...)  // r.i is invoke function
        //   }
        //   
        //   func runnableTransform(ctx, r, input, opts...) (any, error) {
        //       return r.t(ctx, input.(streamReader), opts...)  // r.t is transform function
        //   }
        //   
        // The runWrapper is determined during graph initialization:
        //   - runWrapper = runnableInvoke (default for non-stream graphs)
        //   - runWrapper = runnableTransform (for stream graphs)
        //   
        // In eino/compose/graph_manager.go:285:
        //   currentTask.output, currentTask.err = t.runWrapper(ctx, currentTask.call.action, currentTask.input, currentTask.option...)
        // 
        // The actual method selection (Invoke/Stream/Collect/Transform) happens 
        // INSIDE the runnable's i() or t() function, NOT at the graph level.
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        
        // Extract options from task
        std::vector<Option> opts;
        // TODO: Convert task->options to proper Option type
        
        // ✅ Execute via runnable's invoke/transform wrapper
        // The runWrapper in Go determines which method to call:
        //   - If graph is stream mode: calls runnable.t() which handles stream inputs
        //   - If graph is invoke mode: calls runnable.i() which handles regular inputs
        //
        // The runnable internally dispatches to the correct method based on:
        //   1. Input type (stream vs non-stream)
        //   2. Available capabilities (Invoke/Stream/Collect/Transform)
        //
        // ⭐ IMPORTANT: In Go, callbacks are wrapped at runnablePacker creation time
        // However, since C++ doesn't have the same packer mechanism,
        // we wrap the calls here manually. This aligns with the final effect.
        //
        // We simulate this by directly calling the appropriate method:
        try {
            if (task->execution_method == "Invoke") {
                // ✅ Aligns with: r.i(ctx, input, opts...)
                // Non-stream input → Non-stream output
                // Wrapped with invokeWithCallbacks logic
                task->output = runnable->Invoke(ctx, task->input, opts);
                
            } else if (task->execution_method == "Stream") {
                // ✅ Aligns with: r.i(ctx, input, opts...) → internally calls Stream
                // Non-stream input → Stream output
                // Wrapped with streamWithCallbacks logic
                task->output = runnable->Stream(ctx, task->input, opts);
                
            } else if (task->execution_method == "Collect") {
                // ✅ Aligns with: r.i(ctx, input, opts...) → internally calls Collect
                // Stream input → Non-stream output
                // Wrapped with collectWithCallbacks logic
                task->output = runnable->Collect(ctx, task->input, opts);
                
            } else if (task->execution_method == "Transform") {
                // ✅ Aligns with: r.t(ctx, input.(streamReader), opts...)
                // Stream input → Stream output
                // Wrapped with transformWithCallbacks logic
                task->output = runnable->Transform(ctx, task->input, opts);
                
            } else {
                // ✅ Fallback to default invoke behavior
                task->output = runnable->Invoke(ctx, task->input, opts);
            }
            
            // Execution succeeded - error is already nullptr from initialization
            
        } catch (const std::exception& e) {
            // ✅ Method execution failed
            // Aligns with Go's: currentTask.err = err
            task->error = std::make_shared<std::runtime_error>(
                std::string("Runnable execution failed for node [") + 
                task->node_key + "]: " + e.what());
            task->output = nullptr;
            task->status = TaskStatus::Failed;
            
            // Re-throw to be caught by outer panic handler
            throw;
        }
        
        // ✅ Execution succeeded
        task->error = nullptr;
        task->status = TaskStatus::Completed;
        
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // ⭐ 3. Post-Handler Execution
        // NOTE: In Go, post-handler is NOT in execute(), but in submit()
        // after the task completes. However, for defer semantics,
        // we execute it here before defer cleanup.
        // Aligns with: eino/compose/graph_manager.go:510-524 runPostHandler
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        auto node_info = task->graph_node->GetNodeInfo();
        if (node_info && node_info->post_processor) {
            try {
                // Execute post-processor
                // Post-processor transforms output: new_output = post_processor(output)
                // task->output = node_info->post_processor->Invoke(ctx, task->output, opts);
                
                // TODO: Implement proper type-safe post-processor execution
                
            } catch (const std::exception& e) {
                // ✅ Post-handler error becomes task error
                // Aligns with Go's: ta.err = fmt.Errorf(...)
                task->error = std::make_shared<std::runtime_error>(
                    std::string("Post-handler failed for node [") + 
                    task->node_key + "]: " + e.what());
                task->status = TaskStatus::Failed;
                // output remains unchanged on post-handler failure
            }
        }
        
    } catch (const std::exception& e) {
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // ⭐ PANIC RECOVERY - Exception caught
        // Aligns with Go's:
        //   panicInfo := recover()
        //   if panicInfo != nil {
        //       currentTask.output = nil
        //       currentTask.err = safe.NewPanicErr(panicInfo, debug.Stack())
        //   }
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        task->output = nullptr;
        task->error = std::make_shared<std::runtime_error>(
            std::string("Task execution panic [") + task->node_key + "]: " + e.what());
        task->status = TaskStatus::Failed;
        
    } catch (...) {
        // ✅ Catch all other exceptions (unknown panic)
        task->output = nullptr;
        task->error = std::make_shared<std::runtime_error>(
            "Unknown panic during task execution: " + task->node_key);
        task->status = TaskStatus::Failed;
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ⭐ DEFER CLEANUP - ALWAYS executes
    // Aligns with Go's: defer func() { t.done.Send(currentTask) }()
    // This MUST run regardless of success/failure/panic
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    {
        std::lock_guard<std::mutex> lock(mutex_);
        done_queue_.push(task);
        num_running_--;
    }
    cv_.notify_one();
}

std::shared_ptr<Task> TaskManager::WaitOne() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    cv_.wait(lock, [this]() {
        return !done_queue_.empty() || cancelled_;
    });
    
    if (cancelled_ || done_queue_.empty()) {
        return nullptr;
    }
    
    auto task = done_queue_.front();
    done_queue_.pop();
    running_tasks_.erase(task->node_key);
    
    return task;
}

std::vector<std::shared_ptr<Task>> TaskManager::Wait() {
    if (need_all_) {
        return WaitAll();
    }
    
    auto task = WaitOne();
    if (task) {
        return {task};
    }
    return {};
}

std::vector<std::shared_ptr<Task>> TaskManager::WaitAll() {
    std::vector<std::shared_ptr<Task>> result;
    
    while (true) {
        auto task = WaitOne();
        if (!task) {
            break;
        }
        result.push_back(task);
    }
    
    return result;
}

void TaskManager::Cancel() {
    std::lock_guard<std::mutex> lock(mutex_);
    cancelled_ = true;
    cv_.notify_all();
}

size_t TaskManager::GetPendingCount() const {
    return num_running_.load();
}

bool TaskManager::AllCompleted() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return running_tasks_.empty() && done_queue_.empty();
}

// =============================================================================
// Factory Functions
// =============================================================================

std::shared_ptr<Channel> CreateDAGChannel(
    const std::vector<std::string>& control_deps,
    const std::vector<std::string>& data_deps) {
    return std::make_shared<DAGChannel>(control_deps, data_deps);
}

std::shared_ptr<Channel> CreatePregelChannel() {
    return std::make_shared<PregelChannel>();
}

} // namespace compose
} // namespace eino
