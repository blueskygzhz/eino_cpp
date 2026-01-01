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

#ifndef EINO_CPP_ADK_CONTEXT_H_
#define EINO_CPP_ADK_CONTEXT_H_

// Context and RunContext Management
// ==================================
// Manages execution context for agent workflows, including:
// - RunContext: execution state, run path, session values
// - Context wrapping: storing/retrieving runcontext from void* context pointers
// - Session management: thread-safe access to session values

#include "types.h"
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <string>

namespace eino {
namespace adk {

// Session stores execution-local data during agent run
class RunSession {
public:
    RunSession() = default;
    ~RunSession() = default;

    // Add an event to the session
    void AddEvent(std::shared_ptr<AgentEvent> event);

    // Get all events from the session
    std::vector<std::shared_ptr<AgentEvent>> GetEvents() const;

    // Add a session value
    void AddValue(const std::string& key, void* value);

    // Add multiple session values
    void AddValues(const std::map<std::string, void*>& values);

    // Get a session value
    void* GetValue(const std::string& key) const;

    // Get all session values
    std::map<std::string, void*> GetValues() const;

    // Push interrupt run context
    void PushInterruptRunContext(std::shared_ptr<RunContext> ctx);

    // Get all interrupt run contexts
    std::vector<std::shared_ptr<RunContext>> GetInterruptRunContexts() const;

    // Replace interrupt run context (remove matching paths and add new one)
    void ReplaceInterruptRunContext(std::shared_ptr<RunContext> ctx);

private:
    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<AgentEvent>> events_;
    std::map<std::string, void*> values_;
    std::vector<std::shared_ptr<RunContext>> interrupt_run_contexts_;
};

// RunContext represents the execution context of agents
class ExecutionContext {
public:
    ExecutionContext() = default;
    ~ExecutionContext() = default;

    // Set root input
    void SetRootInput(std::shared_ptr<AgentInput> input);

    // Get root input
    std::shared_ptr<AgentInput> GetRootInput() const;

    // Add a run step
    void AddRunStep(const RunStep& step);

    // Get run path
    std::vector<RunStep> GetRunPath() const;

    // Set run path
    void SetRunPath(const std::vector<RunStep>& path);

    // Get session
    std::shared_ptr<RunSession> GetSession() const;

    // Set session
    void SetSession(std::shared_ptr<RunSession> session);

    // Check if this is root context
    bool IsRoot() const;

    // Deep copy
    std::shared_ptr<ExecutionContext> DeepCopy() const;

private:
    std::shared_ptr<AgentInput> root_input_;
    std::vector<RunStep> run_path_;
    std::shared_ptr<RunSession> session_;
};

// Context utilities for managing execution context in void* pointers
namespace context {

// Initialize new execution context
std::shared_ptr<ExecutionContext> InitializeContext(
    void* ctx,
    const std::string& agent_name,
    std::shared_ptr<AgentInput> input);

// Get execution context from void* context
std::shared_ptr<ExecutionContext> GetExecutionContext(void* ctx);

// Set execution context to void* context
void* SetExecutionContext(void* ctx, std::shared_ptr<ExecutionContext> exec_ctx);

// Create a new execution context with new run session
void* CreateNewExecutionContext(void* ctx);

// Get session from context
std::shared_ptr<RunSession> GetSession(void* ctx);

// Clear execution context
void* ClearExecutionContext(void* ctx);

// Helper to manage context in wrapper
class ContextWrapper {
public:
    ContextWrapper(void* ctx) : original_ctx_(ctx) {}
    
    std::shared_ptr<ExecutionContext> Get() {
        return GetExecutionContext(original_ctx_);
    }
    
    void Set(std::shared_ptr<ExecutionContext> ctx) {
        original_ctx_ = SetExecutionContext(original_ctx_, ctx);
    }

private:
    void* original_ctx_;
};

}  // namespace context

// Utility functions for session management
std::map<std::string, void*> GetSessionValues(void* ctx);
void AddSessionValue(void* ctx, const std::string& key, void* value);
void AddSessionValues(void* ctx, const std::map<std::string, void*>& values);
void* GetSessionValue(void* ctx, const std::string& key);

// Helper for managing interrupt run contexts
void PushInterruptRunContext(void* ctx, std::shared_ptr<ExecutionContext> interrupt_ctx);
void ReplaceInterruptRunContext(void* ctx, std::shared_ptr<ExecutionContext> interrupt_ctx);
std::vector<std::shared_ptr<ExecutionContext>> GetInterruptRunContexts(void* ctx);

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_CONTEXT_H_
