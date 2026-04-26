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

#ifndef EINO_CPP_ADK_MIDDLEWARES_PLANTASK_H_
#define EINO_CPP_ADK_MIDDLEWARES_PLANTASK_H_

// Plan task middleware that provides task management tools for agents.
// Aligned with Go: adk/middlewares/plantask/plantask.go
//
// Adds TaskCreate, TaskGet, TaskUpdate, and TaskList tools to the agent's tool set,
// allowing agents to create and manage structured task lists during coding sessions.

#include <any>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "eino/adk/filesystem/backend.h"
#include "eino/adk/handler.h"

namespace eino {
namespace adk {
namespace middlewares {
namespace plantask {

// Task represents a single task in the plan.
// Aligned with Go: plantask.task
struct Task {
    std::string id;
    std::string subject;
    std::string description;
    std::string status;           // "pending", "in_progress", "completed", "deleted"
    std::vector<std::string> blocks;
    std::vector<std::string> blocked_by;
    std::string active_form;
    std::string owner;
    std::map<std::string, std::any> metadata;
};

// Task status constants
// Aligned with Go: plantask.taskStatus*
static const std::string kTaskStatusPending = "pending";
static const std::string kTaskStatusInProgress = "in_progress";
static const std::string kTaskStatusCompleted = "completed";
static const std::string kTaskStatusDeleted = "deleted";

// DeleteRequest for deleting a file.
struct DeleteRequest {
    std::string file_path;
};

// Backend defines the storage interface for task persistence.
// Aligned with Go: plantask.Backend
class TaskBackend {
public:
    virtual ~TaskBackend() = default;
    virtual std::pair<std::vector<filesystem::FileInfo>, std::string> LsInfo(
        const filesystem::LsInfoRequest& req) = 0;
    virtual std::pair<filesystem::FileContent, std::string> Read(
        const filesystem::ReadRequest& req) = 0;
    virtual std::string Write(const filesystem::WriteRequest& req) = 0;
    virtual std::string Delete(const DeleteRequest& req) = 0;
};

// Config is the configuration for the plantask middleware.
// Aligned with Go: plantask.Config
struct Config {
    std::shared_ptr<TaskBackend> backend;
    std::string base_dir;
};

// New creates a new plantask middleware.
// Aligned with Go: plantask.New()
std::pair<std::shared_ptr<ChatModelAgentMiddleware>, std::string> New(const Config& config);

}  // namespace plantask
}  // namespace middlewares
}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_MIDDLEWARES_PLANTASK_H_
