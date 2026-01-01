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

// RunContext and Session Management
// ==================================
// Aligns with eino/adk/runctx.go
//
// Provides runtime context management for multi-agent execution:
// - RunContext: tracks execution path and session
// - RunSession: stores events and session values
// - Context lifecycle: initialization, propagation, and cleanup

#include "eino/adk/context.h"
#include "eino/adk/types.h"
#include <mutex>
#include <vector>
#include <map>

namespace eino {
namespace adk {

// ============================================================================
// AgentEventWrapper - Wraps AgentEvent with thread-safe message caching
// ============================================================================

class AgentEventWrapper {
public:
    AgentEventWrapper(std::shared_ptr<AgentEvent> event)
        : event_(event), concatenated_message_(nullptr) {}
    
    std::shared_ptr<AgentEvent> GetEvent() const {
        return event_;
    }
    
    schema::Message GetConcatenatedMessage() {
        std::lock_guard<std::mutex> lock(mutex_);
        return concatenated_message_;
    }
    
    void SetConcatenatedMessage(schema::Message msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        concatenated_message_ = msg;
    }

private:
    std::shared_ptr<AgentEvent> event_;
    schema::Message concatenated_message_;
    std::mutex mutex_;
};

// ============================================================================
// RunSession Implementation
// ============================================================================

RunSession::RunSession() {
}

RunSession::~RunSession() {
}

void RunSession::AddEvent(std::shared_ptr<AgentEvent> event) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Wrap event for caching
    auto wrapper = std::make_shared<AgentEventWrapper>(event);
    events_.push_back(event);
}

std::vector<std::shared_ptr<AgentEvent>> RunSession::GetEvents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_;
}

void RunSession::AddValue(const std::string& key, void* value) {
    std::lock_guard<std::mutex> lock(mutex_);
    values_[key] = value;
}

void RunSession::AddValues(const std::map<std::string, void*>& values) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& kv : values) {
        values_[kv.first] = kv.second;
    }
}

void* RunSession::GetValue(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = values_.find(key);
    if (it != values_.end()) {
        return it->second;
    }
    return nullptr;
}

void* RunSession::GetValue(const std::string& key, bool* found) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = values_.find(key);
    if (it != values_.end()) {
        if (found) *found = true;
        return it->second;
    }
    if (found) *found = false;
    return nullptr;
}

std::map<std::string, void*> RunSession::GetValues() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return values_;
}

void RunSession::PushInterruptRunContext(std::shared_ptr<RunContext> ctx) {
    std::lock_guard<std::mutex> lock(mutex_);
    interrupt_run_contexts_.push_back(ctx);
}

std::vector<std::shared_ptr<RunContext>> RunSession::GetInterruptRunContexts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return interrupt_run_contexts_;
}

void RunSession::ReplaceInterruptRunContext(std::shared_ptr<RunContext> ctx) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Remove contexts whose path belongs to the new context
    auto it = interrupt_run_contexts_.begin();
    while (it != interrupt_run_contexts_.end()) {
        // Check if (*it)->run_path belongs to ctx->run_path
        // For simplicity, just append for now
        ++it;
    }
    
    interrupt_run_contexts_.push_back(ctx);
}

// ============================================================================
// ExecutionContext Implementation (renamed from RunContext in context.h)
// ============================================================================

ExecutionContext::ExecutionContext() 
    : session_(std::make_shared<RunSession>()) {
}

ExecutionContext::~ExecutionContext() {
}

void ExecutionContext::SetRootInput(std::shared_ptr<AgentInput> input) {
    root_input_ = input;
}

std::shared_ptr<AgentInput> ExecutionContext::GetRootInput() const {
    return root_input_;
}

void ExecutionContext::AddRunStep(const RunStep& step) {
    run_path_.push_back(step);
}

std::vector<RunStep> ExecutionContext::GetRunPath() const {
    return run_path_;
}

void ExecutionContext::SetRunPath(const std::vector<RunStep>& path) {
    run_path_ = path;
}

std::shared_ptr<RunSession> ExecutionContext::GetSession() const {
    return session_;
}

bool ExecutionContext::IsRoot() const {
    return run_path_.size() == 1;
}

std::unique_ptr<ExecutionContext> ExecutionContext::DeepCopy() const {
    auto copied = std::make_unique<ExecutionContext>();
    copied->root_input_ = root_input_;
    copied->run_path_ = run_path_;
    copied->session_ = session_;  // Session is shared
    return copied;
}

// ============================================================================
// ContextManager Implementation
// ============================================================================

ContextManager& ContextManager::GetInstance() {
    static ContextManager instance;
    return instance;
}

void ContextManager::SetRunSession(void* ctx, std::shared_ptr<RunSession> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[ctx] = session;
}

std::shared_ptr<RunSession> ContextManager::GetRunSession(void* ctx) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(ctx);
    if (it != sessions_.end()) {
        return it->second;
    }
    return nullptr;
}

void ContextManager::ClearContext(void* ctx) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(ctx);
    run_contexts_.erase(ctx);
}

void ContextManager::SetRunContext(void* ctx, std::shared_ptr<ExecutionContext> run_ctx) {
    std::lock_guard<std::mutex> lock(mutex_);
    run_contexts_[ctx] = run_ctx;
}

std::shared_ptr<ExecutionContext> ContextManager::GetRunContext(void* ctx) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = run_contexts_.find(ctx);
    if (it != run_contexts_.end()) {
        return it->second;
    }
    return nullptr;
}

// ============================================================================
// Global Context Helper Functions (align with eino/adk/runctx.go)
// ============================================================================

std::shared_ptr<ExecutionContext> GetRunContext(void* ctx) {
    return ContextManager::GetInstance().GetRunContext(ctx);
}

void SetRunContext(void* ctx, std::shared_ptr<ExecutionContext> run_ctx) {
    ContextManager::GetInstance().SetRunContext(ctx, run_ctx);
}

std::shared_ptr<ExecutionContext> InitRunContext(
    void* ctx,
    const std::string& agent_name,
    std::shared_ptr<AgentInput> input) {
    
    auto run_ctx = GetRunContext(ctx);
    
    if (run_ctx) {
        // Deep copy existing context
        auto new_ctx = run_ctx->DeepCopy();
        run_ctx = std::move(new_ctx);
    } else {
        // Create new context
        run_ctx = std::make_unique<ExecutionContext>();
    }
    
    // Add run step
    RunStep step;
    step.agent_name = agent_name;
    run_ctx->AddRunStep(step);
    
    // Set root input if this is the root
    if (run_ctx->IsRoot()) {
        run_ctx->SetRootInput(input);
    }
    
    // Update context in manager
    SetRunContext(ctx, std::move(run_ctx));
    
    return GetRunContext(ctx);
}

void* ClearRunCtx(void* ctx) {
    ContextManager::GetInstance().ClearContext(ctx);
    return ctx;
}

std::shared_ptr<RunSession> GetSession(void* ctx) {
    auto run_ctx = GetRunContext(ctx);
    if (run_ctx) {
        return run_ctx->GetSession();
    }
    return nullptr;
}

std::vector<std::shared_ptr<RunContext>> GetInterruptRunContexts(void* ctx) {
    auto session = GetSession(ctx);
    if (session) {
        return session->GetInterruptRunContexts();
    }
    return {};
}

void AppendInterruptRunContext(void* ctx, std::shared_ptr<RunContext> interrupt_ctx) {
    auto session = GetSession(ctx);
    if (session) {
        session->PushInterruptRunContext(interrupt_ctx);
    }
}

void ReplaceInterruptRunContext(void* ctx, std::shared_ptr<RunContext> interrupt_ctx) {
    auto session = GetSession(ctx);
    if (session) {
        session->ReplaceInterruptRunContext(interrupt_ctx);
    }
}

}  // namespace adk
}  // namespace eino
