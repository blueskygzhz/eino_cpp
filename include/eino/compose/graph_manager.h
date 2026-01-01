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

#ifndef EINO_CPP_COMPOSE_GRAPH_MANAGER_H_
#define EINO_CPP_COMPOSE_GRAPH_MANAGER_H_

// Aligns with: eino/compose/graph_manager.go
// Contains: channel, channelManager, taskManager implementations

#include <memory>
#include <map>
#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>

namespace eino {
namespace compose {

// Forward declarations
struct Task;

// =============================================================================
// Channel Interface
// Aligns with: eino/compose/graph_manager.go:29-36
// =============================================================================

class Channel {
public:
    virtual ~Channel() = default;
    
    // Report values from predecessors
    virtual void ReportValues(const std::map<std::string, std::shared_ptr<void>>& values) = 0;
    
    // Report control dependencies satisfied
    virtual void ReportDependencies(const std::vector<std::string>& deps) = 0;
    
    // Report skipped nodes
    virtual bool ReportSkip(const std::vector<std::string>& nodes) = 0;
    
    // Get value if channel is ready
    virtual bool Get(bool is_stream, const std::string& node_name,
                    std::shared_ptr<void>& value) = 0;
    
    // Convert values (for checkpoint)
    // Aligned with: eino/compose/graph_manager.go:34 convertValues(fn func(map[string]any) error) error
    virtual void ConvertValues(std::function<void(std::map<std::string, std::shared_ptr<void>>&)> fn) = 0;
    
    // Load from another channel (for checkpoint restore)
    virtual void Load(std::shared_ptr<Channel> other) = 0;
    
    // Set merge configuration
    virtual void SetMergeConfig(const std::map<std::string, std::string>& config) = 0;
    
    // ⭐ Serialization support for checkpoint
    virtual nlohmann::json ToJSON() const = 0;
    static std::shared_ptr<Channel> FromJSON(const nlohmann::json& j);
};

// =============================================================================
// DAG Channel Implementation
// Aligns with: eino/compose/dag.go:50-180
// =============================================================================

enum class DependencyState : uint8_t {
    Waiting = 0,
    Ready = 1,
    Skipped = 2
};

class DAGChannel : public Channel {
public:
    DAGChannel(const std::vector<std::string>& control_deps,
               const std::vector<std::string>& data_deps);
    
    void ReportValues(const std::map<std::string, std::shared_ptr<void>>& values) override;
    void ReportDependencies(const std::vector<std::string>& deps) override;
    bool ReportSkip(const std::vector<std::string>& nodes) override;
    bool Get(bool is_stream, const std::string& node_name,
            std::shared_ptr<void>& value) override;
    void ConvertValues(std::function<void(std::map<std::string, std::shared_ptr<void>>&)> fn) override;
    void Load(std::shared_ptr<Channel> other) override;
    void SetMergeConfig(const std::map<std::string, std::string>& config) override;
    
    // ⭐ Serialization
    nlohmann::json ToJSON() const override;
    
private:
    std::mutex mutex_;
    std::map<std::string, DependencyState> control_predecessors_;
    std::map<std::string, bool> data_predecessors_;
    std::map<std::string, std::shared_ptr<void>> values_;
    bool skipped_ = false;
    std::map<std::string, std::string> merge_config_;
};

// =============================================================================
// Pregel Channel Implementation
// Aligns with: eino/compose/pregel.go:25-90
// =============================================================================

class PregelChannel : public Channel {
public:
    PregelChannel();
    
    void ReportValues(const std::map<std::string, std::shared_ptr<void>>& values) override;
    void ReportDependencies(const std::vector<std::string>& deps) override;
    bool ReportSkip(const std::vector<std::string>& nodes) override;
    bool Get(bool is_stream, const std::string& node_name,
            std::shared_ptr<void>& value) override;
    void ConvertValues(std::function<void(std::map<std::string, std::shared_ptr<void>>&)> fn) override;
    void Load(std::shared_ptr<Channel> other) override;
    void SetMergeConfig(const std::map<std::string, std::string>& config) override;
    
    // ⭐ Serialization
    nlohmann::json ToJSON() const override;
    
private:
    std::mutex mutex_;
    std::map<std::string, std::shared_ptr<void>> values_;
    std::map<std::string, std::string> merge_config_;
};

// =============================================================================
// Channel Manager
// Aligns with: eino/compose/graph_manager.go:115-230
// =============================================================================

class ChannelManager {
public:
    ChannelManager(bool is_stream,
                  const std::map<std::string, std::shared_ptr<Channel>>& channels,
                  const std::map<std::string, std::vector<std::string>>& successors,
                  const std::map<std::string, std::vector<std::string>>& data_predecessors,
                  const std::map<std::string, std::vector<std::string>>& control_predecessors);
    
    // Load channels from checkpoint
    void LoadChannels(const std::map<std::string, std::shared_ptr<Channel>>& channels);
    
    // Update channel values
    void UpdateValues(const std::map<std::string, std::map<std::string, std::shared_ptr<void>>>& values);
    
    // Update control dependencies
    void UpdateDependencies(const std::map<std::string, std::vector<std::string>>& deps);
    
    // Get ready nodes
    std::map<std::string, std::shared_ptr<void>> GetFromReadyChannels();
    
    // Combined update and get
    std::map<std::string, std::shared_ptr<void>> UpdateAndGet(
        const std::map<std::string, std::map<std::string, std::shared_ptr<void>>>& values,
        const std::map<std::string, std::vector<std::string>>& deps);
    
    // Report branch skip
    void ReportBranch(const std::string& from, const std::vector<std::string>& skipped_nodes);
    
    // Get successors for a node
    // Aligns with: accessing successors map in graph_run.go
    std::vector<std::string> GetSuccessors(const std::string& node_name) const {
        auto it = successors_.find(node_name);
        if (it != successors_.end()) {
            return it->second;
        }
        return std::vector<std::string>();
    }
    
    // Get all channels (for checkpoint)
    std::map<std::string, std::shared_ptr<Channel>> GetChannels() const { return channels_; }
    
private:
    bool is_stream_;
    std::map<std::string, std::shared_ptr<Channel>> channels_;
    std::map<std::string, std::vector<std::string>> successors_;
    std::map<std::string, std::vector<std::string>> data_predecessors_;
    std::map<std::string, std::vector<std::string>> control_predecessors_;
};

// =============================================================================
// Task Manager
// Aligns with: eino/compose/graph_manager.go:232-480
// =============================================================================

class TaskManager {
public:
    TaskManager(bool need_all);
    ~TaskManager();
    
    // Submit tasks for execution
    void Submit(const std::vector<std::shared_ptr<Task>>& tasks);
    
    // Wait for one or all tasks to complete
    std::vector<std::shared_ptr<Task>> Wait();
    std::vector<std::shared_ptr<Task>> WaitAll();
    
    // Cancel all running tasks
    void Cancel();
    
    // Get pending count
    size_t GetPendingCount() const;
    
    // Check if all completed
    bool AllCompleted() const;
    
private:
    void Execute(std::shared_ptr<Task> task);
    std::shared_ptr<Task> WaitOne();
    
    bool need_all_;
    std::atomic<uint32_t> num_running_{0};
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::shared_ptr<Task>> done_queue_;
    std::map<std::string, std::shared_ptr<Task>> running_tasks_;
    bool cancelled_ = false;
};

// =============================================================================
// Factory Functions
// =============================================================================

// Create DAG channel builder
std::shared_ptr<Channel> CreateDAGChannel(
    const std::vector<std::string>& control_deps,
    const std::vector<std::string>& data_deps);

// Create Pregel channel builder  
std::shared_ptr<Channel> CreatePregelChannel();

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_GRAPH_MANAGER_H_
