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

#include "../include/eino/adk/context.h"
#include "../include/eino/adk/types.h"
#include <algorithm>

namespace eino {
namespace adk {

// ============================================================================
// RunSession Implementation (aligned with eino runctx.go)
// ============================================================================

void RunSession::AddEvent(std::shared_ptr<AgentEvent> event) {
    std::lock_guard<std::mutex> lock(mutex_);
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
    for (const auto& [k, v] : values) {
        values_[k] = v;
    }
}

void* RunSession::GetValue(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = values_.find(key);
    return it != values_.end() ? it->second : nullptr;
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
    
    // Remove contexts whose run path belongs to the new context
    auto it = interrupt_run_contexts_.begin();
    while (it != interrupt_run_contexts_.end()) {
        const auto& rc = *it;
        if (rc && ctx && ctx->run_path.size() <= rc->run_path.size()) {
            bool belongs_to = true;
            for (size_t i = 0; i < ctx->run_path.size(); ++i) {
                if (ctx->run_path[i].agent_name != rc->run_path[i].agent_name) {
                    belongs_to = false;
                    break;
                }
            }
            if (belongs_to) {
                it = interrupt_run_contexts_.erase(it);
                continue;
            }
        }
        ++it;
    }
    
    interrupt_run_contexts_.push_back(ctx);
}

// ExecutionContext Implementation
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

void ExecutionContext::SetSession(std::shared_ptr<RunSession> session) {
    session_ = session;
}

bool ExecutionContext::IsRoot() const {
    return run_path_.size() == 1;
}

std::shared_ptr<ExecutionContext> ExecutionContext::DeepCopy() const {
    auto copied = std::make_shared<ExecutionContext>();
    copied->root_input_ = root_input_;
    copied->run_path_ = run_path_;
    copied->session_ = session_;
    return copied;
}

// Context utilities - using a context registry pattern
namespace context {

namespace {
    // Thread-local context stack
    thread_local std::map<void*, std::shared_ptr<ExecutionContext>> context_registry_;
}

std::shared_ptr<ExecutionContext> InitializeContext(
    void* ctx,
    const std::string& agent_name,
    std::shared_ptr<AgentInput> input) {
    
    auto exec_ctx = GetExecutionContext(ctx);
    if (!exec_ctx) {
        exec_ctx = std::make_shared<ExecutionContext>();
        exec_ctx->SetSession(std::make_shared<RunSession>());
    } else {
        exec_ctx = exec_ctx->DeepCopy();
    }
    
    exec_ctx->AddRunStep(RunStep{agent_name});
    if (exec_ctx->IsRoot()) {
        exec_ctx->SetRootInput(input);
    }
    
    return exec_ctx;
}

std::shared_ptr<ExecutionContext> GetExecutionContext(void* ctx) {
    auto it = context_registry_.find(ctx);
    if (it != context_registry_.end()) {
        return it->second;
    }
    return nullptr;
}

void* SetExecutionContext(void* ctx, std::shared_ptr<ExecutionContext> exec_ctx) {
    if (exec_ctx) {
        context_registry_[ctx] = exec_ctx;
    } else {
        context_registry_.erase(ctx);
    }
    return ctx;
}

void* CreateNewExecutionContext(void* ctx) {
    auto new_ctx = std::make_shared<ExecutionContext>();
    new_ctx->SetSession(std::make_shared<RunSession>());
    return SetExecutionContext(ctx, new_ctx);
}

std::shared_ptr<RunSession> GetSession(void* ctx) {
    auto exec_ctx = GetExecutionContext(ctx);
    return exec_ctx ? exec_ctx->GetSession() : nullptr;
}

void* ClearExecutionContext(void* ctx) {
    context_registry_.erase(ctx);
    return ctx;
}

}  // namespace context

// Utility functions
std::map<std::string, void*> GetSessionValues(void* ctx) {
    auto exec_ctx = context::GetExecutionContext(ctx);
    if (!exec_ctx) {
        return {};
    }
    auto session = exec_ctx->GetSession();
    return session ? session->GetValues() : std::map<std::string, void*>();
}

void AddSessionValue(void* ctx, const std::string& key, void* value) {
    auto exec_ctx = context::GetExecutionContext(ctx);
    if (exec_ctx) {
        auto session = exec_ctx->GetSession();
        if (session) {
            session->AddValue(key, value);
        }
    }
}

void AddSessionValues(void* ctx, const std::map<std::string, void*>& values) {
    auto exec_ctx = context::GetExecutionContext(ctx);
    if (exec_ctx) {
        auto session = exec_ctx->GetSession();
        if (session) {
            session->AddValues(values);
        }
    }
}

void* GetSessionValue(void* ctx, const std::string& key) {
    auto exec_ctx = context::GetExecutionContext(ctx);
    if (exec_ctx) {
        auto session = exec_ctx->GetSession();
        if (session) {
            return session->GetValue(key);
        }
    }
    return nullptr;
}

void PushInterruptRunContext(void* ctx, std::shared_ptr<ExecutionContext> interrupt_ctx) {
    auto exec_ctx = context::GetExecutionContext(ctx);
    if (exec_ctx) {
        auto session = exec_ctx->GetSession();
        if (session && interrupt_ctx) {
            // Create a RunContext from ExecutionContext
            auto run_ctx = std::make_shared<RunContext>();
            run_ctx->root_input = interrupt_ctx->GetRootInput().get();
            run_ctx->run_path = interrupt_ctx->GetRunPath();
            session->PushInterruptRunContext(run_ctx);
        }
    }
}

void ReplaceInterruptRunContext(void* ctx, std::shared_ptr<ExecutionContext> interrupt_ctx) {
    auto exec_ctx = context::GetExecutionContext(ctx);
    if (exec_ctx) {
        auto session = exec_ctx->GetSession();
        if (session && interrupt_ctx) {
            auto run_ctx = std::make_shared<RunContext>();
            run_ctx->root_input = interrupt_ctx->GetRootInput().get();
            run_ctx->run_path = interrupt_ctx->GetRunPath();
            session->ReplaceInterruptRunContext(run_ctx);
        }
    }
}

std::vector<std::shared_ptr<ExecutionContext>> GetInterruptRunContexts(void* ctx) {
    auto exec_ctx = context::GetExecutionContext(ctx);
    if (exec_ctx) {
        auto session = exec_ctx->GetSession();
        if (session) {
            // Convert RunContext back to ExecutionContext
            auto run_ctxs = session->GetInterruptRunContexts();
            std::vector<std::shared_ptr<ExecutionContext>> result;
            for (const auto& rc : run_ctxs) {
                auto ec = std::make_shared<ExecutionContext>();
                if (rc->root_input) {
                    ec->SetRootInput(std::make_shared<AgentInput>(*rc->root_input));
                }
                ec->SetRunPath(rc->run_path);
                result.push_back(ec);
            }
            return result;
        }
    }
    return {};
}

}  // namespace adk
}  // namespace eino
